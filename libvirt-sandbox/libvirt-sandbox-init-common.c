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

#include <libvirt-sandbox/libvirt-sandbox-config-all.h>
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
#include <mntent.h>
#include <sys/reboot.h>

#include "libvirt-sandbox-rpcpacket.h"

static gboolean debug = FALSE;
static gboolean verbose = FALSE;
static int sigwrite;

#define ATTR_UNUSED __attribute__((__unused__))
static void sync_data(void);
static void umount_fs(void);

static void sig_child(int sig ATTR_UNUSED)
{
    char ignore = '1';
    if (write(sigwrite, &ignore, 1) != 1)
        abort();
}

static gboolean setup_disk_tags(void) {
    FILE *fp;
    gboolean ret = FALSE;
    static char line[1024];
    if (debug)
        fprintf(stderr, "libvirt-sandbox-init-common: %s: populate /dev/disk/by-tag/\n",
                __func__);
    fp = fopen(SANDBOXCONFIGDIR "/disks.cfg", "r");
    if (fp == NULL) {
        fprintf(stderr, "libvirt-sandbox-init-common: %s: cannot open " SANDBOXCONFIGDIR "/disks.cfg: %s\n",
                __func__, strerror(errno));

        goto cleanup;
    }
    if (g_mkdir_with_parents("/dev/disk/by-tag",0755) < 0) {
       fprintf(stderr, "libvirt-sandbox-init-common: %s: cannot create the directory: %s\n",
               __func__, strerror(errno));

       goto cleanup;
    }
    while (fgets(line, sizeof line, fp)) {
        char *tag = line;
        char *device = strchr(tag, '\t');
        gchar *path = NULL;
        *device = '\0';
        device++;
        char *tmp = strchr(device, '\n');
        *tmp = '\0';
        path = g_strdup_printf("/dev/disk/by-tag/%s", tag);

        if (debug)
            fprintf(stderr, "libvirt-sandbox-init-common: %s: %s -> %s\n",
                    __func__, path, device);

        if (symlink(device, path) < 0) {
            fprintf(stderr, "libvirt-sandbox-init-common: %s: cannot create symlink %s -> %s: %s\n",
                    __func__, path, device, strerror(errno));
            g_free(path);
            goto cleanup;
        }

        g_free(path);
    }
    ret = TRUE;
 cleanup:
    if (fp)
        fclose(fp);
    return ret;
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
    const gchar *argv[5];
    size_t nargs = 0;
    gboolean ret;
    sigset_t newset;
    sigset_t oldset;

    argv[nargs++] = "/sbin/dhclient";
    if (debug)
        argv[nargs++] = "-v";
    argv[nargs++] = "--no-pid";
    argv[nargs++] = devname;
    argv[nargs++] = NULL;

    sigemptyset(&newset);
    sigaddset(&newset, SIGHUP);

    sigprocmask(SIG_BLOCK, &newset, &oldset);
    ret = g_spawn_sync(NULL, (gchar**)argv, NULL, 0,
                       NULL, NULL, NULL, NULL, NULL, error);
    sigprocmask(SIG_SETMASK, &oldset, NULL);

    return ret;
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
    gchar *bcaststr = bcast ? g_inet_address_to_string(bcast) : NULL;
    gboolean ret = FALSE;

    const gchar *argv1a[] = {
        "/sbin/ip", "addr",
        "add", fulladdrstr,
        "broadcast", bcaststr,
        "dev", devname,
        NULL
    };
    const gchar *argv1b[] = {
        "/sbin/ip", "addr",
        "add", fulladdrstr,
        "dev", devname,
        NULL
    };
    const gchar *argv2[] = {
        "/sbin/ip", "link",
        "set", devname, "up",
        NULL
    };

    if (bcaststr) {
        if (!g_spawn_sync(NULL, (gchar**)argv1a, NULL, 0,
                          NULL, NULL, NULL, NULL, NULL, error))
            goto cleanup;
    } else {
        if (!g_spawn_sync(NULL, (gchar**)argv1b, NULL, 0,
                          NULL, NULL, NULL, NULL, NULL, error))
            goto cleanup;
    }
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


static gboolean setup_custom_env(GVirSandboxConfig *config, GError **error)
{
    gboolean ret = FALSE;
    GHashTableIter iter;
    gpointer key, value;
    g_hash_table_iter_init (&iter, gvir_sandbox_config_get_envs(config));
    while (g_hash_table_iter_next (&iter, &key, &value)){
        if(setenv(key,value,1)!=0)
            goto cleanup;
    }
    ret = TRUE;
  cleanup:
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

static gboolean run_command(GVirSandboxConfig *config,
                            pid_t *child,
                            int *appin, int *appout, int *apperr)
{
    GVirSandboxConfigInteractive *iconfig = GVIR_SANDBOX_CONFIG_INTERACTIVE(config);
    int pid;
    int master = -1;
    int slave = -1;
    int pipein[2] = { -1, -1};
    int pipeout[2] = { -1, -1};
    int pipeerr[2] = { -1, -1};
    gchar **appargv = gvir_sandbox_config_get_command(config);
    gboolean wanttty = gvir_sandbox_config_interactive_get_tty(iconfig);

    if (debug)
        fprintf(stderr, "libvirt-sandbox-init-common: running command %s %d\n",
                appargv[0], wanttty);

    *appin = *appout = -1;

    if (wanttty) {
        if (openpty(&master, &slave, NULL, NULL, NULL) < 0) {
            fprintf(stderr, "Cannot setup slave for child: %s\n",
                    strerror(errno));
            goto error;
        }

        if (!g_str_equal(gvir_sandbox_config_get_username(config), "root")) {
            if (fchown(slave,
                       gvir_sandbox_config_get_userid(config),
                       gvir_sandbox_config_get_groupid(config)) < 0) {
                fprintf(stderr, "Cannot make PTY available to user: %s\n",
                        strerror(errno));
                goto error;
            }
        }

        *appin = master;
        *appout = master;
        *apperr = master;
    } else {
        if (pipe(pipein) < 0 ||
            pipe(pipeout) < 0 ||
            pipe(pipeerr) < 0) {
            fprintf(stderr, "Cannot open pipe for child: %s\n",
                    strerror(errno));
            goto error;
        }
        *appin = pipein[1];
        *appout = pipeout[0];
        *apperr = pipeerr[0];
    }

    if ((pid = fork()) < 0) {
        fprintf(stderr, "Cannot fork child %s\n", strerror(errno));
        goto error;
    }

    if (pid == 0) {
        if (change_user(gvir_sandbox_config_get_username(config),
                        gvir_sandbox_config_get_userid(config),
                        gvir_sandbox_config_get_groupid(config),
                        gvir_sandbox_config_get_homedir(config)) < 0)
            exit(EXIT_FAILURE);

        if (wanttty) {
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
            close(pipeerr[0]);
            close(STDIN_FILENO);
            close(STDOUT_FILENO);
            close(STDERR_FILENO);

            if (dup2(pipein[0], STDIN_FILENO) != STDIN_FILENO)
                abort();
            if (dup2(pipeout[1], STDOUT_FILENO) != STDOUT_FILENO)
                abort();
            if (dup2(pipeerr[1], STDERR_FILENO) != STDERR_FILENO)
                abort();
        }

        execvp(appargv[0], appargv);
        fprintf(stderr, "Cannot execute '%s': %s\n", appargv[0], strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (wanttty) {
        close(slave);
    } else {
        close(pipein[0]);
        close(pipeout[1]);
        close(pipeerr[1]);
    }

    *child = pid;
    g_strfreev(appargv);
    return TRUE;

 error:
    if (wanttty) {
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
        if (pipeerr[0] < 0)
            close(pipeerr[0]);
        if (pipeerr[1] < 0)
            close(pipeerr[1]);
    }
    *appin = *appout = *apperr = -1;
    g_strfreev(appargv);
    return FALSE;
}

static GVirSandboxRPCPacket *gvir_sandbox_encode_stdout(const gchar *data,
                                                        gsize len,
                                                        unsigned int serial,
                                                        GError **error)
{
    GVirSandboxRPCPacket *pkt = gvir_sandbox_rpcpacket_new(FALSE);

    pkt->header.proc = GVIR_SANDBOX_PROTOCOL_PROC_STDOUT;
    pkt->header.status = GVIR_SANDBOX_PROTOCOL_STATUS_OK;
    pkt->header.type = GVIR_SANDBOX_PROTOCOL_TYPE_DATA;
    pkt->header.serial = serial;

    if (!gvir_sandbox_rpcpacket_encode_header(pkt, error))
        goto error;
    if (!gvir_sandbox_rpcpacket_encode_payload_raw(pkt, data, len, error))
        goto error;

    if (debug)
        fprintf(stderr, "Ready to send %zu %zu %zu\n",
                len, pkt->bufferLength, pkt->bufferOffset);
    return pkt;

 error:
    gvir_sandbox_rpcpacket_free(pkt);
    return NULL;
}


static GVirSandboxRPCPacket *gvir_sandbox_encode_stderr(const gchar *data,
                                                        gsize len,
                                                        unsigned int serial,
                                                        GError **error)
{
    GVirSandboxRPCPacket *pkt = gvir_sandbox_rpcpacket_new(FALSE);

    pkt->header.proc = GVIR_SANDBOX_PROTOCOL_PROC_STDERR;
    pkt->header.status = GVIR_SANDBOX_PROTOCOL_STATUS_OK;
    pkt->header.type = GVIR_SANDBOX_PROTOCOL_TYPE_DATA;
    pkt->header.serial = serial;

    if (!gvir_sandbox_rpcpacket_encode_header(pkt, error))
        goto error;
    if (!gvir_sandbox_rpcpacket_encode_payload_raw(pkt, data, len, error))
        goto error;

    return pkt;

 error:
    gvir_sandbox_rpcpacket_free(pkt);
    return NULL;
}


static GVirSandboxRPCPacket *gvir_sandbox_encode_exit(int status,
                                                      unsigned int serial,
                                                      GError **error)
{
    GVirSandboxRPCPacket *pkt = gvir_sandbox_rpcpacket_new(FALSE);
    GVirSandboxProtocolMessageExit msg;

    memset(&msg, 0, sizeof(msg));
    msg.status = status;

    pkt->header.proc = GVIR_SANDBOX_PROTOCOL_PROC_EXIT;
    pkt->header.status = GVIR_SANDBOX_PROTOCOL_STATUS_OK;
    pkt->header.type = GVIR_SANDBOX_PROTOCOL_TYPE_MESSAGE;
    pkt->header.serial = serial;

    /* The host may destroy the guest any time after receiving
     * the exit code messages. So although the main() has code
     * to sync + unmount we can't rely on that running. So we
     * opportunistically sync + unmount here too.
     */
    sync_data();
    umount_fs();

    if (!gvir_sandbox_rpcpacket_encode_header(pkt, error))
        goto error;
    if (!gvir_sandbox_rpcpacket_encode_payload_msg(pkt,
                                                   (xdrproc_t)xdr_GVirSandboxProtocolMessageExit,
                                                   (void*)&msg,
                                                   error))
        goto error;

    return pkt;

 error:
    gvir_sandbox_rpcpacket_free(pkt);
    return NULL;
}

/* Copied & adapted from libguestfs daemon/sync.c under LGPLv2+ */
static void sync_data(void)
{
  DIR *dir;
  struct dirent *d;
  char *dev_path;
  int fd;

  if (debug)
      fprintf(stderr, "Syncing data\n");
  sync();

  /* On Linux, sync(2) doesn't perform a barrier, so qemu (which may
   * have a writeback cache, even with cache=none) will still have
   * some unwritten data.  Force the data out of any qemu caches, by
   * calling fsync on all block devices.  Note we still need the
   * call to sync above in order to schedule the writes.
   * Thanks to: Avi Kivity, Kevin Wolf.
   */

  if (!(dir = opendir("/dev"))) {
      fprintf(stderr, "opendir: /dev failed %s\n", strerror(errno));
      return;
  }

  for (;;) {
      errno = 0;
      d = readdir(dir);
      if (!d)
          break;

      if (!(g_str_has_prefix(d->d_name, "sd") ||
            g_str_has_prefix(d->d_name, "hd") ||
            g_str_has_prefix(d->d_name, "ubd") ||
            g_str_has_prefix(d->d_name, "vd") ||
            g_str_has_prefix(d->d_name, "sr"))) {
          continue;
      }

      dev_path = g_strdup_printf("/dev/%s", d->d_name);

      if (debug)
          fprintf(stderr, "Syncing fd %s\n", dev_path);
      if ((fd = open(dev_path, O_RDONLY)) < 0) {
          fprintf(stderr, "cannot open %s: %s\n", dev_path,
                  strerror(errno));
          g_free(dev_path);
          continue;
      }

      /* fsync the device. */
      if (debug) {
          fprintf(stderr, "fsync %s\n", dev_path);
      }

      if (fsync(fd) < 0) {
          fprintf(stderr, "failed to fsync %s: %s\n",
                  dev_path, strerror(errno));
      }
      if (close(fd) < 0) {
          fprintf(stderr, "failed to close %s: %s\n",
                  dev_path, strerror(errno));
      }
      g_free(dev_path);
  }

  /* Check readdir didn't fail */
  if (errno != 0) {
      fprintf(stderr, "Failed to read /dev: %s\n",
              strerror(errno));
  }

  /* Close the directory handle */
  if (closedir(dir) < 0) {
      fprintf(stderr, "Failed to block /dev: %s\n",
              strerror(errno));
  }

  if (debug)
      fprintf(stderr, "Syncing complete\n");
}


static int
compare_longest_first (gconstpointer vp1, gconstpointer vp2)
{
    int n1 = strlen(vp1);
    int n2 = strlen(vp2);
    return n2 - n1;
}


/* Copied & adapted from libguestfs daemon/sync.c under LGPLv2+ */
static void umount_fs(void)
{
    FILE *fp;
    struct mntent *m;
    GList *mounts = NULL, *tmp;

    if (debug)
        fprintf(stderr, "Unmounting all filesystems\n");
    if (!(fp = setmntent ("/proc/mounts", "r"))) {
        if (debug)
            fprintf(stderr, "Failed to open /proc/mounts: %s\n",
                    strerror(errno));
        return;
    }

    while ((m = getmntent (fp)) != NULL) {
        if (debug)
            fprintf(stderr, "Got fsname=%s dir=%s type=%s opts=%s freq=%d passno=%d\n",
                    m->mnt_fsname, m->mnt_dir, m->mnt_type, m->mnt_opts,
                    m->mnt_freq, m->mnt_passno);

        mounts = g_list_append(mounts, g_strdup(m->mnt_dir));
    }

    endmntent(fp);

    mounts = g_list_sort(mounts, compare_longest_first);

    /* Unmount them. */
    tmp = mounts;
    while (tmp) {
        char *dir = tmp->data;

        if (debug)
            fprintf(stderr, "Unmounting %s\n", dir);
        if (umount(dir) < 0) {
            /* We expect some failures, so don't pollute
             * logs with them uneccessarily
             */
            if (debug)
                fprintf(stderr, "cannot unmount %s: %s\n",
                        dir, strerror(errno));
            /* ignore failure */
        }
        g_free(dir);

        tmp = tmp->next;
    }

    g_list_free(mounts);
    if (debug)
        fprintf(stderr, "Unmounting complete\n");
}


static gssize read_data(int fd, char *buf, size_t len)
{
    gssize got;

 reread:
    got = read(fd, buf, len);
    if (got < 0) {
        if (errno == EAGAIN)
            return 0;
        if (errno == EINTR)
            goto reread;
        if (debug)
            fprintf(stderr, "Unable to read data: %s\n", strerror(errno));
        return -1;
    }

    return got;
}

static gssize write_data(int fd, const char *buf, size_t len)
{
    gssize got;

 rewrite:
    got = write(fd, buf, len);
    if (got < 0) {
        if (errno == EAGAIN)
            return 0;
        if (errno == EINTR)
            goto rewrite;
        if (debug)
            fprintf(stderr, "Unable to write data: %s\n", strerror(errno));
        return -1;
    }

    return got;
}

typedef enum {
    GVIR_SANDBOX_CONSOLE_STATE_WAITING,
    GVIR_SANDBOX_CONSOLE_STATE_SYNCING,
    GVIR_SANDBOX_CONSOLE_STATE_RUNNING,
} GVirSandboxConsoleState;

static gboolean eventloop(GVirSandboxConfig *config,
                          int sigread,
                          int host)
{
    GVirSandboxRPCPacket *rx = NULL;
    GVirSandboxRPCPacket *tx = NULL;
    gboolean quit = FALSE;
    gboolean appOutEOF = FALSE;
    gboolean appErrEOF = FALSE;
    gboolean appQuit = FALSE;
    int exitstatus = 0;
    gchar *hostToStdin = NULL;
    gsize hostToStdinLength = 0;
    gsize hostToStdinOffset = 0;
    unsigned int serial = 0;
    pid_t child = 0;
    int appin = -1;
    int appout = -1;
    int apperr = -1;
    gboolean ret = FALSE;
    GVirSandboxConsoleState state = GVIR_SANDBOX_CONSOLE_STATE_WAITING;

    if (debug)
        fprintf(stderr, "libvirt-sandbox-init-common: running I/O loop %d %d", appin, appout);


    rx = gvir_sandbox_rpcpacket_new(FALSE);
    rx->bufferLength = 1; /* Ready to get a sync packet */

    while (!quit) {
        int i;
        struct pollfd fds[6];
        size_t nfds = 0;
        int appinEv = 0;
        int appoutEv = 0;
        int apperrEv = 0;
        int hostEv = 0;

        fds[nfds].fd = sigread;
        fds[nfds].events = POLLIN;
        nfds++;

        switch (state) {
        case GVIR_SANDBOX_CONSOLE_STATE_WAITING:
            hostEv = POLLIN;
            break;
        case GVIR_SANDBOX_CONSOLE_STATE_SYNCING:
            hostEv = POLLIN;
            if (tx)
                hostEv |= POLLOUT;
            break;
        case GVIR_SANDBOX_CONSOLE_STATE_RUNNING:
            if (hostToStdin && appin != -1)
                appinEv |= POLLOUT;
            else if (rx != NULL)
                hostEv |= POLLIN;

            if (tx != NULL)
                hostEv |= POLLOUT;
            else {
                if (!appOutEOF && appout != -1)
                    appoutEv |= POLLIN;
                if ((appout != apperr) && !appErrEOF && apperr != -1)
                    apperrEv |= POLLIN;
            }
            break;
        default:
            break;
        }

        if (appinEv) {
            fds[nfds].fd = appin;
            fds[nfds].events = appinEv;
            nfds++;
        }
        if (appoutEv) {
            fds[nfds].fd = appout;
            fds[nfds].events = appoutEv;
            nfds++;
        }
        if (apperrEv) {
            fds[nfds].fd = apperr;
            fds[nfds].events = apperrEv;
            nfds++;
        }
        if (hostEv) {
            fds[nfds].fd = host;
            fds[nfds].events = hostEv;
            nfds++;
        }

    repoll:
        if (poll(fds, nfds, -1) < 0) {
            if (errno == EINTR)
                goto repoll;
            if (debug)
                fprintf(stderr, "Poll error:%s\n",
                        strerror(errno));
            return -1;
        }

        for (i = 0 ; i < nfds ; i++) {
            gssize got;

            if (fds[i].fd == sigread) {
                /* The self-pipe signal handler */
                if (fds[i].revents) {
                    char ignore;
                    pid_t rv;
                    if (read(sigread, &ignore, 1) != 1)
                        goto cleanup;
                    while (1) {
                        rv = waitpid(-1, &exitstatus, WNOHANG);
                        if (rv == -1 || rv == 0)
                            break;
                        if (rv == child) {
                            appQuit = TRUE;
                            if (appErrEOF && appOutEOF) {
                                if (debug)
                                    fprintf(stderr, "Encoding exit status sigchild %d\n", exitstatus);
                                if (!(tx = gvir_sandbox_encode_exit(exitstatus, serial++, NULL)))
                                    goto cleanup;
                            }
                        }
                    }
                }
            } else if (fds[i].fd == host) {
                /* The channel to the virt-sandbox library client in host */
                if (fds[i].revents & POLLIN) {
                    if (debug)
                        fprintf(stderr, "host readable\n");
                    if (rx) {
                    readmore:
                        got = read_data(host,
                                        rx->buffer + rx->bufferOffset,
                                        rx->bufferLength - rx->bufferOffset);
                        if (debug)
                            fprintf(stderr, "read %zd %zu %zu\n", got, rx->bufferLength, rx->bufferOffset);
                        if (got <= 0) {
                            gvir_sandbox_rpcpacket_free(rx);
                            rx = NULL;
                            quit = TRUE;
                        } else {
                            rx->bufferOffset += got;
                            if (rx->bufferLength == rx->bufferOffset) {
                                switch (state) {
                                case GVIR_SANDBOX_CONSOLE_STATE_WAITING:
                                    /* We now expect a 'wait' byte. Anything else is bad */
                                    if (rx->buffer[0] != GVIR_SANDBOX_PROTOCOL_HANDSHAKE_WAIT) {
                                        if (debug)
                                            fprintf(stderr, "Unexpected syntax byte %d",
                                                    rx->buffer[0]);
                                        goto cleanup;
                                    }
                                    state = GVIR_SANDBOX_CONSOLE_STATE_SYNCING;
                                    if (debug)
                                        fprintf(stderr, "Sending sync confirm\n");

                                    /* Great, we can sync with the host now */
                                    tx = gvir_sandbox_rpcpacket_new(FALSE);
                                    tx->buffer[0] = GVIR_SANDBOX_PROTOCOL_HANDSHAKE_SYNC;
                                    tx->bufferLength = 1;
                                    tx->bufferOffset = 0;

                                    rx->bufferLength = 1;
                                    rx->bufferOffset = 0;
                                    break;

                                case GVIR_SANDBOX_CONSOLE_STATE_SYNCING:
                                    /* We now expect a 'sync' byte. We might still get a few
                                     * 'wait' bytes which we need to ignore. Anything else is bad
                                     */
                                    if (rx->buffer[0] != GVIR_SANDBOX_PROTOCOL_HANDSHAKE_WAIT &&
                                        rx->buffer[0] != GVIR_SANDBOX_PROTOCOL_HANDSHAKE_SYNC) {
                                        if (debug)
                                            fprintf(stderr, "Unexpected syntax byte %d",
                                                    rx->buffer[0]);
                                        goto cleanup;
                                    }
                                    /* We've got a 'sync' from the host. Now we can launch
                                     * the command we know neither side will loose any I/O
                                     */
                                    if (rx->buffer[0] == GVIR_SANDBOX_PROTOCOL_HANDSHAKE_SYNC) {
                                        if (debug)
                                            fprintf(stderr, "Running command\n");
                                        if (!run_command(config,
                                                         &child,
                                                         &appin,
                                                         &appout,
                                                         &apperr)) {
                                            if (debug)
                                                fprintf(stderr, "Failed to run command\n");
                                            goto cleanup;
                                        }
                                        state = GVIR_SANDBOX_CONSOLE_STATE_RUNNING;
                                        rx->bufferLength = 4;
                                        rx->bufferOffset = 0;
                                    } else {
                                        if (debug)
                                            fprintf(stderr, "Ignoring delayed wait\n");
                                        rx->bufferLength = 1;
                                        rx->bufferOffset = 0;
                                    }
                                    break;

                                case GVIR_SANDBOX_CONSOLE_STATE_RUNNING:
                                    if (debug)
                                        fprintf(stderr, "Read packet %zu\n", rx->bufferLength);
                                    if (rx->bufferLength == GVIR_SANDBOX_PROTOCOL_LEN_MAX) {
                                        GError *error = NULL;
                                        if (!gvir_sandbox_rpcpacket_decode_length(rx, &error)) {
                                            if (debug)
                                                fprintf(stderr, "Cannot decode length %zu: %s\n",
                                                        rx->bufferLength, error->message);
                                            goto cleanup;
                                        }
                                        goto readmore;
                                    } else {
                                        if (!gvir_sandbox_rpcpacket_decode_header(rx, NULL)) {
                                            if (debug)
                                                fprintf(stderr, "Cannot decode header %zu\n", rx->bufferLength);
                                            goto cleanup;
                                        }

                                        switch (rx->header.proc) {
                                        case GVIR_SANDBOX_PROTOCOL_PROC_STDIN:
                                            if (rx->bufferLength - rx->bufferOffset) {
                                                hostToStdinOffset = 0;
                                                hostToStdinLength = rx->bufferLength - rx->bufferOffset;
                                                hostToStdin = g_new0(gchar, hostToStdinLength);
                                                memcpy(hostToStdin,
                                                       rx->buffer + rx->bufferOffset,
                                                       hostToStdinLength);
                                                if (debug)
                                                    fprintf(stderr, "Processed stdin %zu\n", hostToStdinLength);
                                            } else {
                                                close(appin);
                                                appin = -1;
                                            }
                                            break;

                                        case GVIR_SANDBOX_PROTOCOL_PROC_QUIT:
                                            quit = TRUE;
                                            break;

                                        case GVIR_SANDBOX_PROTOCOL_PROC_STDOUT:
                                        case GVIR_SANDBOX_PROTOCOL_PROC_STDERR:
                                        case GVIR_SANDBOX_PROTOCOL_PROC_EXIT:
                                        default:
                                            if (debug)
                                                fprintf(stderr, "Unexpected proc %u\n", rx->header.proc);
                                            goto cleanup;
                                        }
                                    }
                                    gvir_sandbox_rpcpacket_free(rx);
                                    rx = NULL;
                                    break;
                                default:
                                    if (debug)
                                        fprintf(stderr, "Unexpected state %d\n", state);
                                    break;
                                }
                            }
                        }
                    }
                    fds[i].revents &= ~(POLLIN);
                }
                if (fds[i].revents & POLLOUT) {
                    if (debug)
                        fprintf(stderr, "Host writable\n");
                    if (tx) {
                        got = write_data(host,
                                         tx->buffer + tx->bufferOffset,
                                         tx->bufferLength - tx->bufferOffset);
                        if (got < 0) {
                            if (debug)
                                fprintf(stderr, "Cannot write packet to host %s\n",
                                        strerror(errno));
                            gvir_sandbox_rpcpacket_free(tx);
                            tx = NULL;
                            quit = TRUE;
                        } else {
                            tx->bufferOffset += got;
                            if (tx->bufferOffset == tx->bufferLength) {
                                if (debug)
                                    fprintf(stderr, "Wrote packet %zu to host\n",
                                            tx->bufferOffset);
                                gvir_sandbox_rpcpacket_free(tx);
                                tx = NULL;
                            }
                        }
                    }
                    fds[i].revents &= ~(POLLOUT);
                }
                if (fds[i].revents) {
                    quit = TRUE;
                }
            } else if (fds[i].fd == appin &&
                       fds[i].fd == appout) {
                /* The child application, when using a psuedo-tty */
                if (fds[i].revents & POLLIN) {
                    if (!tx) {
                        gsize len = 4096;
                        gchar *buf = g_new0(gchar, len);
                        got = read_data(appout, buf, len);
                        if (got <= 0) {
                            if (got < 0 && debug)
                                fprintf(stderr, "Failed to read from app %s\n",
                                        strerror(errno));
                            appOutEOF = TRUE;
                            appErrEOF = TRUE;
                            if (appQuit) {
                                if (debug)
                                    fprintf(stderr, "Encoding exit status appout tty %d\n", exitstatus);
                                if (!(tx = gvir_sandbox_encode_exit(exitstatus, serial++, NULL)))
                                    goto cleanup;
                            }
                        } else {
                            if (!(tx = gvir_sandbox_encode_stdout(buf, got, serial++, NULL))) {
                                g_free(buf);
                                if (debug)
                                    fprintf(stderr, "Failed to encode stdout\n");
                                goto cleanup;
                            }
                        }
                        g_free(buf);
                    }
                    fds[i].revents &= ~(POLLIN | POLLHUP);
                }
                if (fds[i].revents & POLLOUT) {
                    if (hostToStdin) {
                        got = write_data(appin,
                                         hostToStdin + hostToStdinOffset,
                                         hostToStdinLength - hostToStdinOffset);
                        if (got < 0) {
                            if (debug)
                                fprintf(stderr, "Failed to write to app %s\n",
                                        strerror(errno));
                            g_free(hostToStdin);
                            hostToStdin = NULL;
                            hostToStdinLength = hostToStdinOffset = 0;
                        } else {
                            hostToStdinOffset += got;
                            if (hostToStdinOffset == hostToStdinLength) {
                                g_free(hostToStdin);
                                hostToStdin = NULL;
                                hostToStdinLength = hostToStdinOffset = 0;
                                rx = gvir_sandbox_rpcpacket_new(TRUE);
                            }
                        }
                    }
                    fds[i].revents &= ~(POLLOUT | POLLHUP);
                }
                if (fds[i].revents & POLLHUP) {
                    appOutEOF = TRUE;
                    appErrEOF = TRUE;
                    if (appQuit) {
                        if (debug)
                            fprintf(stderr, "Encoding exit status due to HUP %d\n", exitstatus);
                        if (!(tx = gvir_sandbox_encode_exit(exitstatus, serial++, NULL)))
                            goto cleanup;
                    }
                }
            } else if (fds[i].fd == appin) {
                /* The child stdin when using a plain pipe */
                if (fds[i].revents && hostToStdin) {
                    got = write_data(appin,
                                     hostToStdin + hostToStdinOffset,
                                     hostToStdinLength - hostToStdinOffset);
                    if (got < 0) {
                        g_free(hostToStdin);
                        hostToStdin = NULL;
                        hostToStdinLength = hostToStdinOffset = 0;
                    } else {
                        hostToStdinOffset += got;
                        if (hostToStdinOffset == hostToStdinLength) {
                            g_free(hostToStdin);
                            hostToStdin = NULL;
                            hostToStdinLength = hostToStdinOffset = 0;
                            rx = gvir_sandbox_rpcpacket_new(TRUE);
                        }
                    }
                }
            } else if (fds[i].fd == appout) {
                /* The child stdout when using a plain pipe */
                if (fds[i].revents && !tx) {
                    if (!tx) {
                        gsize len = 4096;
                        gchar *buf = g_new0(gchar, len);
                        got = read_data(appout, buf, len);
                        if (got <= 0) {
                            appOutEOF = TRUE;
                            if (appErrEOF && appQuit) {
                                if (debug)
                                    fprintf(stderr, "Encoding exit status appout %d\n", exitstatus);
                                if (!(tx = gvir_sandbox_encode_exit(exitstatus, serial++, NULL)))
                                    goto cleanup;
                            }
                        } else {
                            if (!(tx = gvir_sandbox_encode_stdout(buf, got, serial++, NULL))) {
                                g_free(buf);
                                goto cleanup;
                            }
                        }
                        g_free(buf);
                    }
                }
            } else if (fds[i].fd == apperr) {
                /* The child stderr when using a plain pipe */
                if (fds[i].revents && !tx) {
                    if (!tx) {
                        gsize len = 4096;
                        gchar *buf = g_new0(gchar, len);
                        got = read_data(apperr, buf, len);
                        if (got <= 0) {
                            appErrEOF = TRUE;
                            if (appOutEOF && appQuit) {
                                if (debug)
                                    fprintf(stderr, "Encoding exit status apperr %d\n", exitstatus);
                                if (!(tx = gvir_sandbox_encode_exit(exitstatus, serial++, NULL)))
                                    goto cleanup;
                            }
                        } else {
                            if (!(tx = gvir_sandbox_encode_stderr(buf, got, serial++, NULL))) {
                                g_free(buf);
                                goto cleanup;
                            }
                        }
                        g_free(buf);
                    }
                }
            }
        }
    }

    ret = TRUE;

 cleanup:
    if (appin != -1) {
        close(appin);
        if (appin == appout)
            appout = -1;
        if (appin == apperr)
            apperr = -1;
    }
    if (appout != -1)
        close(appout);
    if (apperr != -1)
        close(apperr);
    return ret;
}


static int
run_interactive(GVirSandboxConfig *config)
{
    int sigpipe[2] = { -1, -1 };
    int host = -1;
    int ret = -1;
    struct termios  rawattr;
    const char *devname;

    if (pipe(sigpipe) < 0) {
        g_printerr(_("libvirt-sandbox-init-common: unable to create signal pipe: %s"),
                   strerror(errno));
        return -1;
    }

    sigwrite = sigpipe[1];
    signal(SIGCHLD, sig_child);

    /* XXX lame hack */
    if (getenv("LIBVIRT_LXC_NAME")) {
        if (gvir_sandbox_config_get_shell(config))
            devname = "/dev/tty3";
        else
            devname = "/dev/tty2";
    } else {
        devname = "/dev/hvc0";
    }

    if ((host = open(devname, O_RDWR)) < 0) {
        g_printerr(_("libvirt-sandbox-init-common: cannot open %s: %s"),
                   devname, strerror(errno));
        goto cleanup;
    }

    tcgetattr(STDIN_FILENO, &rawattr);
    cfmakeraw(&rawattr);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &rawattr);

    tcgetattr(host, &rawattr);
    cfmakeraw(&rawattr);
    tcsetattr(host, TCSAFLUSH, &rawattr);

    if (!eventloop(config,
                   sigpipe[0],
                   host))
        goto cleanup;

    ret = 0;

 cleanup:
    signal(SIGCHLD, SIG_DFL);

    if (sigpipe[0] != -1)
        close(sigpipe[0]);
    if (sigpipe[1] != -1)
        close(sigpipe[1]);
    if (host != -1)
        close(host);

    return ret;
}


static int
run_service(GVirSandboxConfig *config)
{
    gchar **command = gvir_sandbox_config_get_command(config);

    if (change_user(gvir_sandbox_config_get_username(config),
                    gvir_sandbox_config_get_userid(config),
                    gvir_sandbox_config_get_groupid(config),
                    gvir_sandbox_config_get_homedir(config)) < 0)
        return -1;

    execv(command[0], (char**)command);
    g_printerr(_("libvirt-sandbox-init-common: %s: cannot execute %s: %s\n"),
               __func__, command[0], strerror(errno));
    return -1;
}


static void libvirt_sandbox_version(void)
{
    g_print(_("%s version %s\n"), PACKAGE, VERSION);

    exit(EXIT_SUCCESS);
}


int main(int argc, char **argv) {
    gchar *configfile = NULL;
    gboolean poweroff = FALSE;
    GError *error = NULL;
    GOptionContext *context;
    GOptionEntry options[] = {
        { "version", 'V', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
          libvirt_sandbox_version, N_("display version information"), NULL },
        { "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose,
          N_("display verbose information"), NULL },
        { "debug", 'd', 0, G_OPTION_ARG_NONE, &debug,
          N_("display debugging information"), NULL },
        { "config", 'c', 0, G_OPTION_ARG_STRING, &configfile,
          N_("config file path"), "URI"},
        { "poweroff", 'p', 0, G_OPTION_ARG_NONE, &poweroff,
          N_("clean power off when exiting"), NULL},
        { NULL, 0, 0, G_OPTION_ARG_NONE, NULL, NULL, NULL }
    };
    const char *help_msg = N_("Run '" PACKAGE " --help' to see a full list of available command line options");
    GVirSandboxConfig *config;
    int ret = EXIT_FAILURE;

    setlocale(LC_ALL, "");
    bindtextdomain(PACKAGE, LOCALEDIR);
    bind_textdomain_codeset(PACKAGE, "UTF-8");
    textdomain(PACKAGE);

    if (geteuid() != 0) {
        g_printerr(_("%s: must be launched as root\n"), argv[0]);
        exit(EXIT_FAILURE);
    }

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
        g_printerr(_("%s: Unable to load config %s: %s\n"),
                   argv[0],
                   configfile ? configfile :
                   SANDBOXCONFIGDIR "/sandbox.cfg",
                   error->message);
        g_error_free(error);
        goto cleanup;
    }

    setenv("PATH", "/bin:/usr/bin:/usr/local/bin:/sbin/:/usr/sbin", 1);
    unsetenv("LD_LIBRARY_PATH");

    if (gvir_sandbox_config_get_shell(config) &&
        start_shell() < 0)
        exit(EXIT_FAILURE);

    if (!setup_disk_tags())
        exit(EXIT_FAILURE);

    if (!setup_custom_env(config, &error))
        goto error;

    if (!setup_network(config, &error))
        goto error;

    if (GVIR_SANDBOX_IS_CONFIG_INTERACTIVE(config)) {
        if (run_interactive(config) < 0)
            goto cleanup;
    } else if (GVIR_SANDBOX_IS_CONFIG_SERVICE(config)) {
        if (run_service(config) < 0)
            goto cleanup;
    } else {
        GVirSandboxConfigClass *klass = GVIR_SANDBOX_CONFIG_GET_CLASS(config);
        g_printerr(_("Unsupported configuration type %s"),
                   g_type_name(G_TYPE_FROM_CLASS(klass)));
        goto cleanup;
    }

    ret = EXIT_SUCCESS;

 cleanup:
    if (error)
        g_error_free(error);

    sync_data();

    if (poweroff) {
        umount_fs();
        reboot(RB_POWER_OFF);
        /* Should not be reached, but if it is, kernel will panic anyway */
    }
    return ret;

 error:
    g_printerr("%s: %s",
               argv[0],
               error && error->message ? error->message : _("Unknown failure"));
    goto cleanup;
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
