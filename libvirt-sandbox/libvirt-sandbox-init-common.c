/*
 * libvirt-sandbox-init-common.c: common guest startup
 *
 * Copyright (C) 2011 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Daniel P. Berrange <berrange@redhat.com>
 */

#include <config.h>

#include <libvirt-sandbox/libvirt-sandbox.h>
#include <glib/gi18n.h>

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_CAPNG
#include <cap-ng.h>
#endif
#include <pwd.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <pty.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <poll.h>
#include <termios.h>
#include <unistd.h>
#include <limits.h>
#include <grp.h>
#include <sys/reboot.h>

#define MAGIC "xoqpuɐs"

static gboolean debug = FALSE;
static gboolean verbose = FALSE;
static gboolean poweroff = FALSE;
static int sigwrite;

static G_GNUC_NORETURN void doexit(int status)
{
    if (poweroff) {
        sync();
        reboot(RB_POWER_OFF);
        perror("reboot");
    }
    exit(status);
}

#define ATTR_UNUSED __attribute__((__unused__))

#define GVIR_SANDBOX_INIT_COMMON_ERROR gvir_sandbox_init_common_error_quark()

static GQuark
gvir_sandbox_init_common_error_quark(void)
{
    return g_quark_from_static_string("gvir-sandbox-init-common");
}

static void sig_child(int sig ATTR_UNUSED)
{
    char ignore = '1';
    if (write(sigwrite, &ignore, 1) != 1)
        abort();
}

static ssize_t
safewrite(int fd, const void *buf, size_t count)
{
    size_t nwritten = 0;
    while (count > 0) {
        ssize_t r = write(fd, buf, count);

        if (r < 0 && errno == EINTR)
            continue;
        if (r < 0)
            return r;
        if (r == 0)
            return nwritten;
        buf = (const char *)buf + r;
        count -= r;
        nwritten += r;
    }
    return nwritten;
}


static int
start_shell(void)
{
    pid_t pid;

    const gchar *devname = "/dev/ttyS1";
    if (access(devname, R_OK|W_OK) < 0) {
        devname = "/dev/tty2";
    }

    if (debug)
        fprintf(stderr, "libvirt-sandbox-init-common: starting shell %s\n", devname);

    pid = fork();
    if (pid < 0) {
        fprintf(stderr, "libvirt-sandbox-init-common: %s: cannot fork: %s\n",
                __func__, strerror(errno));
        return -1;
    }

    if (pid == 0) {
        const char *shellargv[] = { "/bin/sh", NULL };
        setsid();

        int tty = open(devname, O_RDWR);

        if (tty < 0) {
            fprintf(stderr, "libvirt-sandbox-init-common: %s: cannot open %s: %s\n",
                    __func__, devname, strerror(errno));
            exit(EXIT_FAILURE);
        }

        dup2(tty, STDIN_FILENO);
        dup2(tty, STDOUT_FILENO);
        dup2(tty, STDERR_FILENO);

        close(tty);

        execv(shellargv[0], (char**)shellargv);
        fprintf(stderr, "libvirt-sandbox-init-common: %s: cannot execute %s: %s\n",
                __func__, shellargv[0], strerror(errno));
        exit(EXIT_FAILURE);
    }

    return 0;
}


static gboolean start_dhcp(const gchar *devname, GError **error)
{
    const gchar *argv[] = { "/sbin/dhclient", "--no-pid", devname, NULL };

    if (!g_spawn_async(NULL, (gchar**)argv, NULL, 0,
                       NULL, NULL, NULL, error))
        return FALSE;

    return TRUE;
}


static gboolean add_address(const gchar *devname,
                            GVirSandboxConfigNetworkAddress *config,
                            GError **error)
{
    GInetAddress *addr = gvir_sandbox_config_network_address_get_primary(config);
    guint prefix = gvir_sandbox_config_network_address_get_prefix(config);
    GInetAddress *bcast = gvir_sandbox_config_network_address_get_broadcast(config);
    gchar *addrstr = g_inet_address_to_string(addr);
    gchar *fulladdrstr = g_strdup_printf("%s/%u", addrstr, prefix);
    gchar *bcaststr = g_inet_address_to_string(bcast);
    gboolean ret = FALSE;

    const gchar *argv1[] = {
        "/sbin/ip", "addr",
        "add", fulladdrstr,
        "broadcast", bcaststr,
        "dev", devname,
        NULL
    };
    const gchar *argv2[] = {
        "/sbin/ip", "link",
        "set", devname, "up",
        NULL
    };

    if (!g_spawn_sync(NULL, (gchar**)argv1, NULL, 0,
                      NULL, NULL, NULL, NULL, NULL, error))
        goto cleanup;
    if (!g_spawn_sync(NULL, (gchar**)argv2, NULL, 0,
                      NULL, NULL, NULL, NULL, NULL, error))
        goto cleanup;

    ret = TRUE;
cleanup:
    g_free(addrstr);
    g_free(bcaststr);
    g_free(fulladdrstr);
    return ret;
}


static gboolean add_route(const gchar *devname,
                          GVirSandboxConfigNetworkRoute *config,
                          GError **error)
{
    guint prefix = gvir_sandbox_config_network_route_get_prefix(config);
    GInetAddress *gateway = gvir_sandbox_config_network_route_get_gateway(config);
    GInetAddress *target = gvir_sandbox_config_network_route_get_target(config);
    gchar *targetstr = g_inet_address_to_string(target);
    gchar *fulltargetstr = g_strdup_printf("%s/%u", targetstr, prefix);
    gchar *gatewaystr = g_inet_address_to_string(gateway);
    gboolean ret = FALSE;

    const gchar *argv[] = {
        "/sbin/ip", "route",
        "add", fulltargetstr,
        "via", gatewaystr,
        "dev", devname,
        NULL
    };

    if (!g_spawn_sync(NULL, (gchar**)argv, NULL, 0,
                      NULL, NULL, NULL, NULL, NULL, error))
        goto cleanup;

    ret = TRUE;
cleanup:
    g_free(fulltargetstr);
    g_free(gatewaystr);
    g_free(targetstr);
    return ret;
}


static gboolean setup_network_device(GVirSandboxConfigNetwork *config,
                                     const gchar *devname,
                                     GError **error)
{
    GList *addrs = NULL;
    GList *routes = NULL;
    GList *tmp;
    gboolean ret = FALSE;

    if (gvir_sandbox_config_network_get_dhcp(config)) {
        if (!start_dhcp(devname, error))
            goto cleanup;
    } else {
        tmp = addrs = gvir_sandbox_config_network_get_addresses(config);
        while (tmp) {
            GVirSandboxConfigNetworkAddress *addr = tmp->data;

            if (!add_address(devname, addr, error))
                goto cleanup;

            tmp = tmp->next;
        }

        tmp = routes = gvir_sandbox_config_network_get_routes(config);
        while (tmp) {
            GVirSandboxConfigNetworkRoute *route = tmp->data;

            if (!add_route(devname, route, error))
                goto cleanup;

            tmp = tmp->next;
        }
    }

    ret = TRUE;

cleanup:
    g_list_foreach(addrs, (GFunc)g_object_unref, NULL);
    g_list_free(addrs);
    g_list_foreach(routes, (GFunc)g_object_unref, NULL);
    g_list_free(routes);
    return ret;
}



static gboolean setup_network(GVirSandboxConfig *config, GError **error)
{
    int i = 0;
    GList *nets, *tmp;
    gchar *devname = NULL;
    gboolean ret = FALSE;

    nets = tmp = gvir_sandbox_config_get_networks(config);

    while (tmp) {
        GVirSandboxConfigNetwork *netconfig = tmp->data;

        g_free(devname);
        devname = g_strdup_printf("eth%d", i++);
        if (!setup_network_device(netconfig, devname, error))
            goto cleanup;

        tmp = tmp->next;
    }

    ret = TRUE;

cleanup:
    g_free(devname);
    g_list_foreach(nets, (GFunc)g_object_unref, NULL);
    g_list_free(nets);
    return ret;
}


static gboolean setup_bind_mount(GVirSandboxConfigMount *config, GError **error)
{
    const gchar *src = gvir_sandbox_config_mount_get_root(config);
    const gchar *tgt = gvir_sandbox_config_mount_get_target(config);

    if (mount(src, tgt, NULL, MS_BIND, NULL) < 0) {
        g_set_error(error, GVIR_SANDBOX_INIT_COMMON_ERROR, 0,
                    "Cannot bind %s to %s", src, tgt);
        return FALSE;
    }

    return TRUE;
}


static gboolean setup_bind_mounts(GVirSandboxConfig *config, GError **error)
{
    GList *mounts, *tmp;
    gboolean ret = FALSE;

    mounts = tmp = gvir_sandbox_config_get_guest_bind_mounts(config);

    while (tmp) {
        GVirSandboxConfigMount *mntconfig = tmp->data;

        if (!setup_bind_mount(mntconfig, error))
            goto cleanup;

        tmp = tmp->next;
    }

    ret = TRUE;

cleanup:
    g_list_foreach(mounts, (GFunc)g_object_unref, NULL);
    g_list_free(mounts);
    return ret;
}


static int change_user(const gchar *user,
                       uid_t uid,
                       gid_t gid,
                       const gchar *home)
{
    if (debug)
        fprintf(stderr, "libvirt-sandbox-init-common: changing user %s %d %d %s\n",
                user, uid, gid, home);

    if (g_str_equal(user, "root"))
        return 0;

    setenv("HOME", home, 1);
    setenv("USER", user, 1);
    setenv("USERNAME", user, 1);
    setenv("LOGNAME", user, 1);

#ifdef HAVE_CAPNG
    capng_clear(CAPNG_SELECT_BOTH);
    /* XXX refactor this code so in child process everything is dropped,
     * while this helper just returns BOOT*/
    capng_update(CAPNG_ADD, CAPNG_EFFECTIVE|CAPNG_PERMITTED, CAP_SYS_BOOT|CAP_SETUID);

    if (capng_change_id(uid, gid,
                        CAPNG_DROP_SUPP_GRP | CAPNG_CLEAR_BOUNDING)) {
        fprintf(stderr, "Cannot change ID to %d:%d: %s\n",
                uid, gid, strerror(errno));
        return -1;
    }
#else
    if (setgroups(0, NULL) < 0) {
        fprintf(stderr, "Cannot clear supplementary groups IDs: %s\n",
                strerror(errno));
        return -1;
    }
    if (setregid(gid, gid) < 0) {
        fprintf(stderr, "Cannot change group ID to %d: %s\n",
                gid, strerror(errno));
        return -1;
    }
    if (setreuid(uid, uid) < 0) {
        fprintf(stderr, "Cannot change user ID to %d: %s\n",
                uid, strerror(errno));
        return -1;
    }
#endif

    if (chdir(home) < 0) {
        if (chdir("/") < 0) {
            fprintf(stderr, "Cannot change working directory to %s or /: %s",
                    home, strerror(errno));
            return -1;
        }
    }

    return 0;
}

static int run_command(gboolean interactive, gchar **argv,
                       int *appin, int *appout)
{
    int pid;
    int master = -1;
    int slave = -1;
    int pipein[2] = { -1, -1};
    int pipeout[2] = { -1, -1};

    if (debug)
        fprintf(stderr, "libvirt-sandbox-init-common: running command %s %d\n",
                argv[0], interactive);

    *appin = *appout = -1;

    if (interactive) {
        if (openpty(&master, &slave, NULL, NULL, NULL) < 0) {
            fprintf(stderr, "Cannot setup slave for child: %s\n",
                    strerror(errno));
            goto error;
        }

        *appin = master;
        *appout = master;
    } else {
        if (pipe(pipein) < 0 ||
            pipe(pipeout) < 0) {
            fprintf(stderr, "Cannot open pipe for child: %s\n",
                    strerror(errno));
            goto error;
        }
        *appin = pipein[1];
        *appout = pipeout[0];
    }

    if ((pid = fork()) < 0) {
        fprintf(stderr, "Cannot fork child %s\n", strerror(errno));
        goto error;
    }

    if (pid == 0) {
        if (interactive) {
            close(master);
            close(STDIN_FILENO);
            close(STDOUT_FILENO);
            close(STDERR_FILENO);

            if (dup2(slave, STDIN_FILENO) != STDIN_FILENO)
                abort();
            if (dup2(slave, STDOUT_FILENO) != STDOUT_FILENO)
                abort();
            if (dup2(slave, STDERR_FILENO) != STDERR_FILENO)
                abort();

            if (setsid() < 0) {
                fprintf(stderr, "Cannot start new session ID: %s\n", strerror(errno));
                abort();
            }
        } else {
            close(pipein[1]);
            close(pipeout[0]);
            close(STDIN_FILENO);
            close(STDOUT_FILENO);
            close(STDERR_FILENO);

            if (dup2(pipein[0], STDIN_FILENO) != STDIN_FILENO)
                abort();
            if (dup2(pipeout[1], STDOUT_FILENO) != STDOUT_FILENO)
                abort();
            if (dup2(pipeout[1], STDERR_FILENO) != STDERR_FILENO)
                abort();
        }

        if (safewrite(STDOUT_FILENO, MAGIC, sizeof(MAGIC)) != sizeof(MAGIC)) {
            fprintf(stderr, "Cannot send handshake magic %s\n", MAGIC);
            abort();
        }

        execv(argv[0], argv);
        fprintf(stderr, "Cannot execute '%s': %s\n", argv[0], strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (interactive)
        close(slave);
    else {
        close(pipein[0]);
        close(pipeout[1]);
    }

    return pid;

error:
    if (interactive) {
        if (master != -1)
            close(master);
        if (slave != -1)
            close(slave);
    } else {
        if (pipein[0] < 0)
            close(pipein[0]);
        if (pipein[1] < 0)
            close(pipein[1]);
        if (pipeout[0] < 0)
            close(pipeout[0]);
        if (pipeout[1] < 0)
            close(pipeout[1]);
    }
    *appin = *appout = -1;
    return -1;
}

static ssize_t read_data(int fd, char *buf, size_t *len, size_t max)
{
    size_t avail = max - *len;
    ssize_t got;

    got = read(fd, buf + *len, avail);
    if (got < 0) {
        if (errno == EAGAIN)
            return 0;
        fprintf(stderr, "Unable to read data: %s\n", strerror(errno));
        return -1;
    }

    *len += got;

    return got;
}

static ssize_t write_data(int fd, char *buf, size_t *len)
{
    ssize_t got;

    got = write(fd, buf, *len);
    if (got < 0) {
        if (errno == EAGAIN)
            return 0;
        fprintf(stderr, "Unable to write data: %s\n", strerror(errno));
        return -1;
    }

    memmove(buf, buf+got, *len-got);
    *len -= got;

    return got;
}


static int run_io(pid_t child, int sigread, int appin, int appout)
{
    char hostToApp[1024];
    size_t hostToAppLen = 0;
    char appToHost[1024];
    size_t appToHostLen = 0;

    int hostin = STDIN_FILENO;
    int hostout = STDOUT_FILENO;

    int inescape = 0;

    if (debug)
        fprintf(stderr, "libvirt-sandbox-init-common: running I/O loop %d %d", appin, appout);

    while (1) {
        int i;
        struct pollfd fds[6];
        size_t nfds = 0;

        fds[nfds].fd = sigread;
        fds[nfds].events = POLLIN;
        nfds++;

        /* If hostin is still open & we have space to store data */
        if (hostin != -1 && hostToAppLen < sizeof(hostToApp)) {
            fds[nfds].fd = hostin;
            fds[nfds].events = POLLIN;
            nfds++;
        }

        /* If hostout is still open & we have pending data to send */
        if (hostout != -1 && appToHostLen > 0) {
            fds[nfds].fd = hostout;
            fds[nfds].events = POLLOUT;
            nfds++;
        }

        /* If app is using a PTY & still open */
        if ((appin == appout) && appin != -1) {
            fds[nfds].events = 0;
            /* If we have data to transmit & don't have an escape seq */
            if (hostToAppLen > 0 && !inescape)
                fds[nfds].events |= POLLOUT;

            /* If we have space to received more data */
            if (appToHostLen < sizeof(appToHost))
                fds[nfds].events |= POLLIN;

            if (fds[nfds].events != 0) {
                fds[nfds].fd = appin;
                nfds++;
            }
        }

        /* If app is using a pair of pipes */
        if (appin != appout) {
            /* If appin is still open & we have pending data & don't have an escape seq */
            if (appin != -1 && hostToAppLen > 0 && !inescape) {
                fds[nfds].fd = appin;
                fds[nfds].events |= POLLOUT;
                nfds++;
            }

            /* If appout is still open & we have space for more data */
            if (appout != -1 && appToHostLen < sizeof(appToHost)) {
                fds[nfds].fd = appout;
                fds[nfds].events = POLLIN;
                nfds++;
            }
        }

        /* This shouldn't actually happen, but for some reason... */
        if (nfds == 0)
            break;

    repoll:
        if (poll(fds, nfds, -1) < 0) {
            if (errno == EINTR)
                goto repoll;
            fprintf(stderr, "Poll error:%s\n",
                    strerror(errno));
            return -1;
        }

        for (i = 0 ; i < nfds ; i++) {
            ssize_t got;

            if (fds[i].fd == sigread) {
                if (fds[i].revents) {
                    char ignore;
                    if (read(sigread, &ignore, 1) != 1)
                        abort();
                    waitpid(child, NULL, WNOHANG);
                }
            } else if (fds[i].fd == hostin) {
                if (fds[i].revents) {
                    got = read_data(hostin, hostToApp, &hostToAppLen, sizeof(hostToApp));
                    if (got <= 0) { /* On EOF or error, close */
                        close(hostin);
                        hostin = -1;
                    }
                }
            } else if (fds[i].fd == hostout) {
                if (fds[i].revents) {
                    got = write_data(hostout, appToHost, &appToHostLen);
                    if (got < 0) {
                        close(hostout);
                        hostout = -1;
                    }
                }
            } else if (fds[i].fd == appin && fds[i].fd == appout) {
                if (fds[i].revents & POLLIN) {
                    got = read_data(appin, appToHost, &appToHostLen, sizeof(appToHost));
                    if (got <= 0) {
                        close(appin);
                        appin = appout = -1;
                    }
                    fds[i].revents &= ~(POLLIN);
                }
                if (fds[i].revents & POLLOUT) {
                    got = write_data(appin, hostToApp, &hostToAppLen);
                    if (got < 0) {
                        close(appin);
                        appin = appout = -1;
                    }
                    fds[i].revents &= ~(POLLOUT);
                }
                if (fds[i].revents) {
                    close(appin);
                    appin = appout = -1;
                }
            } else if (fds[i].fd == appin) {
                if (fds[i].revents) {
                    got = write_data(appin, hostToApp, &hostToAppLen);
                    if (got < 0) {
                        close(appin);
                        appin = -1;
                    }
                }
            } else if (fds[i].fd == appout) {
                if (fds[i].revents) {
                    got = read_data(appout, appToHost, &appToHostLen, sizeof(appToHost));
                    if (got <= 0) {
                        close(appout);
                        appout = -1;
                    }
                }
            }
        }

        int n;
        for (n = 0, i = 0 ; i < hostToAppLen ; i++) {
            if (inescape) {
                if (hostToApp[i] == '9') {
                    close(hostin);
                    hostin = -1;
                } else {
                    hostToApp[n] = hostToApp[i];
                    n++;
                }
                inescape = 0;
            } else {
                if (hostToApp[i] == '\\') {
                    inescape = 1;
                } else {
                    if (n < i)
                        hostToApp[n] = hostToApp[i];
                    n++;
                }
            }
        }
        hostToAppLen = n;

        if ((hostin == -1) &&
            (hostToAppLen == 0) &&
            (appin != -1)) {
            close(appin);
            if (appin == appout)
                appout = -1;
            appin = -1;
        }
        if ((appout == -1) &&
            (appToHostLen == 0) &&
            (hostout != -1)) {
            close(hostout);
            hostout = -1;
        }

        /* See if we've closed all streams */
        if (hostin == -1 &&
            hostout == -1 &&
            appout == -1 &&
            appin == -1)
            break;

        /* If child has gone away we've no more data to send back to host */
        if ((kill(child, 0) < 0) &&
            (appToHostLen == 0))
            break;
    }

    if (hostin != -1)
        close(hostin);
    if (hostout != -1)
        close(hostout);
    if (appin != -1)
        close(appout);
    if (appout != -1)
        close(appout);
    return 0;
}


static int
run_interactive(GVirSandboxConfigInteractive *config)
{
    pid_t pid;
    int sigpipe[2] = { -1, -1 };
    int ret = -1;
    int appin;
    int appout;
    struct termios  rawattr;

    tcgetattr(STDIN_FILENO, &rawattr);
    cfmakeraw(&rawattr);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &rawattr);

    if (pipe(sigpipe) < 0) {
        g_printerr("libvirt-sandbox-init-common: unable to create signal pipe: %s",
                   strerror(errno));
        return -1;
    }

    sigwrite = sigpipe[1];
    signal(SIGCHLD, sig_child);

    if ((pid = run_command(gvir_sandbox_config_interactive_get_tty(config),
                           gvir_sandbox_config_interactive_get_command(config),
                           &appin, &appout)) < 0)
        goto cleanup;

    if (debug)
        g_printerr("Got %d %d\n", appin, appout);

    if (run_io(pid, sigpipe[0], appin, appout) < 0)
        goto cleanup;

    ret = 0;

cleanup:
    signal(SIGCHLD, SIG_DFL);

    if (sigpipe[0] != -1)
        close(sigpipe[0]);
    if (sigpipe[1] != -1)
        close(sigpipe[1]);

    return ret;
}


static int
run_service(GVirSandboxConfigService *config)
{
    const char *initargv[] = {
        "/bin/systemd",
        "--unit",
        "sandbox.service",
        "--system",
        NULL,
    };
    const char *shargv[] = {
        "/bin/sh",
        NULL,
    };

    if (mount(SANDBOXCONFIGDIR "/systemd", "/lib/systemd/system", NULL, MS_BIND, NULL) < 0) {
        g_printerr("%s", "Cannot bind " SANDBOXCONFIGDIR "/systemd to /lib/systemd/system");
        return FALSE;
    }

    if (mount(SANDBOXCONFIGDIR "/empty", "/etc/systemd/system", NULL, MS_BIND, NULL) < 0) {
        g_printerr("%s", "Cannot bind " SANDBOXCONFIGDIR "/empty to /etc/systemd/system");
        return FALSE;
    }

    if (debug)
        execv(shargv[0], (char **)shargv);
    else
        execv(initargv[0], (char**)initargv);
    g_printerr("libvirt-sandbox-init-common: %s: cannot execute %s: %s\n",
               __func__, debug ? shargv[0] : initargv[0], strerror(errno));
    return -1;
}


static void libvirt_sandbox_version(void)
{
    g_print(_("%s version %s\n"), PACKAGE, VERSION);

    exit(EXIT_SUCCESS);
}


int main(int argc, char **argv) {
    gchar *configfile = NULL;
    GError *error = NULL;
    GOptionContext *context;
    GOptionEntry options[] = {
        { "version", 'V', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
          libvirt_sandbox_version, N_("display version information"), NULL },
        { "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose,
          N_("display verbose information"), NULL },
        { "poweroff", 'p', 0, G_OPTION_ARG_NONE, &poweroff,
          N_("poweroff machine after completion"), NULL },
        { "debug", 'd', 0, G_OPTION_ARG_NONE, &debug,
          N_("display debugging information"), NULL },
        { "config", 'c', 0, G_OPTION_ARG_STRING, &configfile,
          N_("config file path"), "URI"},
        { NULL, 0, 0, G_OPTION_ARG_NONE, NULL, NULL, NULL }
    };
    const char *help_msg = N_("Run '" PACKAGE " --help' to see a full list of available command line options");
    GVirSandboxConfig *config;
    int ret = EXIT_FAILURE;

    if (geteuid() != 0) {
        g_printerr("%s: must be launched as root\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (!gvir_sandbox_init_check(&argc, &argv, &error))
        exit(EXIT_FAILURE);

    g_set_application_name(_("Libvirt Sandbox Init Common"));

    context = g_option_context_new (_("- Libvirt Sandbox"));
    g_option_context_add_main_entries (context, options, NULL);
    g_option_context_parse (context, &argc, &argv, &error);
    if (error) {
        g_printerr("%s: %s\n%s\n",
                   argv[0],
                   error->message,
                   gettext(help_msg));
        g_error_free(error);
        goto cleanup;
    }

    g_option_context_free(context);

    if (!(config = gvir_sandbox_config_load_from_path(configfile ? configfile :
                                                      SANDBOXCONFIGDIR "/sandbox.cfg", &error))) {
        g_printerr("%s: Unable to load config %s: %s\n",
                   argv[0],
                   configfile ? configfile :
                   SANDBOXCONFIGDIR "/sandbox.cfg",
                   error->message);
        g_error_free(error);
        goto cleanup;
    }

    setenv("PATH", "/bin:/usr/bin:/usr/local/bin:/sbin/:/usr/sbin", 1);

    if (gvir_sandbox_config_get_shell(config) &&
        start_shell() < 0)
        exit(EXIT_FAILURE);

    if (!setup_network(config, &error))
        goto error;

    if (!setup_bind_mounts(config, &error))
        goto error;

    if (change_user(gvir_sandbox_config_get_username(config),
                    gvir_sandbox_config_get_userid(config),
                    gvir_sandbox_config_get_groupid(config),
                    gvir_sandbox_config_get_homedir(config)) < 0)
        exit(EXIT_FAILURE);

    if (GVIR_SANDBOX_IS_CONFIG_INTERACTIVE(config)) {
        if (run_interactive(GVIR_SANDBOX_CONFIG_INTERACTIVE(config)) < 0)
            goto cleanup;
    } else if (GVIR_SANDBOX_IS_CONFIG_SERVICE(config)) {
        if (run_service(GVIR_SANDBOX_CONFIG_SERVICE(config)) < 0)
            goto cleanup;
    } else {
        GVirSandboxConfigClass *klass = GVIR_SANDBOX_CONFIG_GET_CLASS(config);
        g_printerr("Unsupported configuration type %s",
                   g_type_name(G_TYPE_FROM_CLASS(klass)));
        goto cleanup;
    }

    ret = EXIT_SUCCESS;

cleanup:
    if (error)
        g_error_free(error);

    doexit(ret);

error:
    g_printerr("%s: %s",
               argv[0],
               error && error->message ? error->message : "Unknown failure");
    goto cleanup;
}
