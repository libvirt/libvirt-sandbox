/*
 * virt-sandbox-service-util.c: libvirt sandbox service util command
 *
 * Copyright (C) 2012-2013 Red Hat, Inc.
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
 * Author: Daniel J Walsh <dwalsh@redhat.com>
 * Author: Daniel P. Berrange <berrange@redhat.com>
 */

#define _GNU_SOURCE
#include <config.h>
#include <limits.h>

#include <libvirt-sandbox/libvirt-sandbox.h>
#include <glib/gi18n.h>
#include <dirent.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/wait.h>
#include <errno.h>
#define STREQ(x,y) (strcmp(x,y) == 0)

#ifdef HAVE_LIBSELINUX
#include <selinux/selinux.h>
#endif

static gboolean do_close(GVirSandboxConsole *con G_GNUC_UNUSED,
                         gboolean error G_GNUC_UNUSED,
                         gpointer opaque)
{
    GMainLoop *loop = opaque;
    g_main_loop_quit(loop);
    return FALSE;
}

static int makeargv(const char *s, char ***argvp) {
    gchar *ptr;
    int argc = -1;
    int i;
    const gchar *delim=" \t";
    if ((!s) || strlen(s) == 0)
        return argc;

    while(s && isspace(*s))
        s++;

    if ((!s) || strlen(s) == 0)
        return argc;

    ptr = g_strdup(s);
    if (!ptr)
        return argc;

    if (strtok(ptr, delim) == NULL)
        argc = 0;
    else
        for (argc = 1; strtok(NULL, delim) != NULL;
             argc++);

    strcpy(ptr, s);
    if ((*argvp = calloc(argc + 1, sizeof(char *))) == NULL) {
        free(ptr);
        argc = -1;
    } else {            /* insert pointers to tokens into the array */
        if (argc > 0) {
            **argvp = strtok(ptr, delim);
            for (i = 1; i < argc; i++)
                *(*argvp + i) = strtok(NULL, delim);
        } else {
            **argvp = NULL;
            free(ptr);
        }
    }

    return argc;
}

static void libvirt_sandbox_version(void)
{
    g_print(_("%s version %s\n"), PACKAGE, VERSION);
    exit(EXIT_SUCCESS);
}

/*
   Join the namespace of the pid file.
   Note:
   All namespace FDs must be open before doing any setns calls.  Otherwise path to the
   /proc/PID/ns will change out from under the current process, and open calls will fail.
*/
static int join_namespace(pid_t pid) {
    int ret = -1;
    DIR *dir = NULL;
    struct dirent *entry;
    int fds[100]; /* Can't see us going over 100 namespaces */
    int i = 0;
    char path[1024];
    int len = sizeof(fds);
    int size = sizeof(path);

    memset(fds, -1, len);

    snprintf(path, size, "/proc/%d/ns", pid);
    if ((dir = opendir(path)) == NULL) {
        g_printerr(_("Failed to open container process path %s\n"), path);
        goto cleanup;
    }
    while (((entry = readdir(dir)) != NULL) && (i < len)) {
        /* skip '.' and '..' */
        if (*entry->d_name == '.')
            continue;

        snprintf(path, size, "/proc/%d/ns/%s", pid, entry->d_name);
        fds[i] = open(path, O_RDONLY);
        if (fds[i] < 0) {
            g_printerr(_("Failed to open container namespace path %s: %m\n"), path);
            len = i;
            goto cleanup;
        }
        i++;
    }
    len = i;
    for (i=0; ((i < len)) ; i++) {
        if (setns(fds[i],0) < 0) {
            g_printerr(_("Failed to setup namespace: %m\n"));
            goto cleanup;
        }
    }
    ret = 0;

cleanup:
    for (i=0; i < len; i++) {
            if ( fds[i] > -1 )
                    close(fds[i]);
    }
    if (dir)
        closedir(dir);

    return ret;
}

/*
  Set process context to match labels within the container.
*/
static int set_process_label(pid_t pid) {
    int ret = 0;

#ifdef HAVE_LIBSELINUX

    security_context_t execcon = NULL;

    if (is_selinux_enabled() > 0) {
        if (getpidcon(pid, &execcon) < 0) {
            g_printerr(_("Unable to get process context for pid %d\n"), pid);
            ret = -1;
        } else {
            if (setexeccon(execcon) < 0) {
                g_printerr(_("Unable to set executable context for pid %d\n"), pid);
                ret = -1;
            }
            freecon(execcon);
        }
        if (ret < 0) {
            /* If SELinux in permissive mode ignore error */
            if (security_getenforce() == 0) {
                ret = 0;
            }
        }
    }

#endif

    return ret;
}

/*
  This function should not require the PID to be passed in.  Eventually we
  should be able to query the libvirt for this information to get the pid of
  libvirt_lxc or systemd associated with the container.
*/
static int container_execute( GVirSandboxContext *ctx, const gchar *command, pid_t pid ) {

    int ret = EXIT_FAILURE;
    char **argv = NULL;
    int argc=-1;
    int i;

    if (set_process_label(pid) < 0)
        goto cleanup;

    /* need to get pid from libvirt for container */
    join_namespace(pid);

    argc = makeargv(command, &argv);
    if (argc > -1) {
        int child = fork();
        if (child) {
            int stat_loc;
            ret = wait(&stat_loc);
            if (ret < 0) {
                g_printerr(_("Unable to wait for child %s\n"), strerror(errno));
                goto cleanup;
            }
            ret = WIFEXITED(stat_loc);
            if (ret) {
                ret = WEXITSTATUS(stat_loc);
                if (ret) {
                    g_printerr(_("Failed to execute %s: %s\n"), command, strerror(ret));
                }
            }
        } else {
            execv(argv[0],&argv[0]);
            exit(errno);
        }
    }

cleanup:
    for (i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);

    return ret;
}

static int container_start( GVirSandboxContext *ctx, GMainLoop *loop ) {

    int ret = EXIT_FAILURE;
    GError *err = NULL;
    GVirSandboxConsole *con = NULL;

    if (!(gvir_sandbox_context_start(ctx, &err))) {
        g_printerr(_("Unable to start container: %s\n"),
                   err && err->message ? err->message : _("unknown"));
        return ret;
    }

    if (!(con = gvir_sandbox_context_get_log_console(ctx, &err)))  {
        g_printerr(_("Unable to get log console for container: %s\n"),
                   err && err->message ? err->message : _("unknown"));
        return ret;
    }

    g_signal_connect(con, "closed", (GCallback)do_close, loop);

    if (gvir_sandbox_console_attach_stderr(con, &err) < 0) {
        g_printerr(_("Unable to attach console to stderr in the container: %s\n"),
                   err && err->message ? err->message : _("unknown"));
        return ret;
    }

    g_main_loop_run(loop);

    return EXIT_SUCCESS;
}

static int container_attach( GVirSandboxContext *ctx,  GMainLoop *loop ) {

    GError *err = NULL;
    GVirSandboxConsole *con = NULL;
    int ret = EXIT_FAILURE;

    if (!(gvir_sandbox_context_attach(ctx, &err))) {
        g_printerr(_("Unable to attach to container: %s\n"),
                   err && err->message ? err->message : _("unknown"));
        return ret;
    }

    if (!(con = gvir_sandbox_context_get_shell_console(ctx, &err)))  {
        g_printerr(_("Unable to get shell console for container: %s\n"),
                   err && err->message ? err->message : _("unknown"));
        return ret;
    }

    g_signal_connect(con, "closed", (GCallback)do_close, loop);

    if (!(gvir_sandbox_console_attach_stdio(con, &err))) {
        g_printerr(_("Unable to attach to container: %s\n"),
                   err && err->message ? err->message : _("unknown"));
        return ret;
    }

    g_main_loop_run(loop);

    return EXIT_SUCCESS;
}

static int container_stop( GVirSandboxContext *ctx, GMainLoop *loop G_GNUC_UNUSED) {

    GError *err = NULL;

    if (!(gvir_sandbox_context_attach(ctx, &err))) {
        g_printerr(_("Unable to attach to container: %s\n"),
                   err && err->message ? err->message : _("unknown"));
        return EXIT_FAILURE;
    }

    if (!(gvir_sandbox_context_stop(ctx, &err))) {
        g_printerr(_("Unable to stop container: %s\n"),
                   err && err->message ? err->message : _("unknown"));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static int (*container_func)( GVirSandboxContext *ctx, GMainLoop *loop ) = NULL;

static gboolean libvirt_lxc_start(const gchar *option_name,
                                  const gchar *value,
                                  const gpointer *data,
                                  const GError **error)

{
    if (container_func) return FALSE;
    container_func = container_start;
    return TRUE;
}

static gboolean libvirt_lxc_stop(const gchar *option_name,
                                 const gchar *value,
                                 const gpointer *data,
                                 const GError **error)

{
    if (container_func) return FALSE;
    container_func = container_stop;
    return TRUE;
}

static gboolean libvirt_lxc_attach(const gchar *option_name,
                                   const gchar *value,
                                   const gpointer *data,
                                   const GError **error)

{
    if (container_func) return FALSE;
    container_func = container_attach;
    return TRUE;
}

int main(int argc, char **argv) {
    GMainLoop *loop = NULL;
    GVirSandboxConfig *config = NULL;
    GVirSandboxContext *ctx = NULL;
    GError *err = NULL;
    GVirConnection *hv = NULL;
    int ret = EXIT_FAILURE;
    pid_t pid = 0;
    gchar *buf=NULL;
    gchar *uri = NULL;
    gchar *command = NULL;

    gchar **cmdargs = NULL;
    GOptionContext *context;
    GOptionEntry options[] = {
        { "version", 'V', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
          libvirt_sandbox_version, N_("Display version information"), NULL },
        { "start", 's', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
          libvirt_lxc_start, N_("Start a container"), NULL },
        { "stop", 'S', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
          libvirt_lxc_stop, N_("Stop a container"), NULL },
        { "attach", 'a', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
          libvirt_lxc_attach, N_("Attach to a container"), NULL },
        { "execute", 'e', 0, G_OPTION_ARG_STRING, &command,
          N_("Execute command in a container"), NULL },
        { "pid", 'p', 0, G_OPTION_ARG_INT, &pid,
          N_("Pid of process in container to which the command will run"), "PID"},
        { "connect", 'c', 0, G_OPTION_ARG_STRING, &uri,
          N_("Connect to hypervisor Default:'lxc:///'"), "URI"},
        { G_OPTION_REMAINING, '\0', 0, G_OPTION_ARG_STRING_ARRAY, &cmdargs,
          NULL, "CONTAINER_NAME" },
        { NULL, 0, 0, G_OPTION_ARG_NONE, NULL, NULL, NULL }
    };
    const char *help_msg = N_("Run 'virt-sandbox-service-util --help' to see a full list of available command line options\n");

    if (!gvir_sandbox_init_check(&argc, &argv, &err))
        exit(EXIT_FAILURE);

    context = g_option_context_new (_("- Libvirt Sandbox Service"));
    g_option_context_add_main_entries (context, options, NULL);
    g_option_context_parse (context, &argc, &argv, &err);

    if (err) {
        g_printerr("%s\n%s\n",
                   err->message,
                   gettext(help_msg));
        goto cleanup;
    }

    if ( container_func == NULL && command == NULL ) {
        g_printerr(_("Invalid command: You must specify --start, --stop, --execute or --attach\n%s"),
                   gettext(help_msg));
        goto cleanup;
    }

    if (!cmdargs || !cmdargs[0] ) {
        g_printerr(_("Invalid command CONTAINER_NAME required: %s"),
                   gettext(help_msg));
        goto cleanup;
    }

    if ( command && (pid == 0)) {
        g_printerr(_("Invalid command: You must only one of specify a pid with --execute\n%s"),
                   gettext(help_msg));
        goto cleanup;
    }

    g_option_context_free(context);

    g_set_application_name(_("Libvirt Sandbox Service"));

    buf = g_strdup_printf("/etc/libvirt-sandbox/services/%s.sandbox", cmdargs[0]);
    if (!buf) {
        g_printerr(_("Out of Memory\n"));
        goto cleanup;
    }

    if (uri)
        hv = gvir_connection_new(uri);
    else
        hv = gvir_connection_new("lxc:///");

    if (!hv) {
        g_printerr(_("error opening connect to lxc:/// \n"));
        goto cleanup;
    }

    if (!gvir_connection_open(hv, NULL, &err)) {
        g_printerr(_("Unable to open connection: %s\n"),
                   err && err->message ? err->message : _("unknown"));
        goto cleanup;
    }

    if(!(config = gvir_sandbox_config_load_from_path(buf, &err))) {
        g_printerr(_("Unable to read config file %s: %s\n"), buf,
                   err && err->message ? err->message : _("unknown"));
        goto cleanup;
    }

    if (GVIR_SANDBOX_IS_CONFIG_INTERACTIVE(config)) {
        GVirSandboxContextInteractive *service;
        if (!(service = gvir_sandbox_context_interactive_new(hv, GVIR_SANDBOX_CONFIG_INTERACTIVE(config)))) {
            g_printerr(_("Unable to create new context service: %s\n"),
                       err && err->message ? err->message : _("unknown"));
            goto cleanup;
        }
        ctx = GVIR_SANDBOX_CONTEXT(service);
    } else {
        GVirSandboxContextService *service;

        if (!(service = gvir_sandbox_context_service_new(hv, GVIR_SANDBOX_CONFIG_SERVICE(config)))) {
            g_printerr(_("Unable to create new context service: %s\n"),
                       err && err->message ? err->message : _("unknown"));
            goto cleanup;
        }
        ctx = GVIR_SANDBOX_CONTEXT(service);
    }

    if (command) {
        container_execute(ctx, command, pid);
    }
    else {
        loop = g_main_loop_new(g_main_context_default(), 1);
        ret = container_func(ctx, loop);
    }

cleanup:
    if (hv)
        gvir_connection_close(hv);

    free(buf);

    if (config)
        g_object_unref(config);

    exit(ret);
}
