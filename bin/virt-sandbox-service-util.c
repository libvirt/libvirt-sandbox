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

#include <config.h>

#include <libvirt-sandbox/libvirt-sandbox.h>
#include <glib/gi18n.h>

#define STREQ(x,y) (strcmp(x,y) == 0)

static gboolean do_close(GVirSandboxConsole *con G_GNUC_UNUSED,
                         gboolean error G_GNUC_UNUSED,
                         gpointer opaque)
{
    GMainLoop *loop = opaque;
    g_main_loop_quit(loop);
    return FALSE;
}


static void libvirt_sandbox_version(void)
{
    g_print(_("%s version %s\n"), PACKAGE, VERSION);
    exit(EXIT_SUCCESS);
}


static GVirSandboxContext *libvirt_sandbox_get_context(const char *uri,
                                                       const char *name)
{
    GVirSandboxConfig *config = NULL;
    GVirSandboxContextService *ctx = NULL;
    GError *err = NULL;
    GVirConnection *conn = NULL;
    gchar *configfile = NULL;

    configfile = g_strdup_printf("/etc/libvirt-sandbox/services/%s/config/sandbox.cfg", name);

    if (uri)
        conn = gvir_connection_new(uri);
    else
        conn = gvir_connection_new("lxc:///");

    if (!gvir_connection_open(conn, NULL, &err)) {
        g_printerr(_("Unable to open connection: %s\n"),
                   err && err->message ? err->message : _("unknown"));
        goto cleanup;
    }

    if (!(config = gvir_sandbox_config_load_from_path(configfile, &err))) {
        g_printerr(_("Unable to read config file %s: %s\n"), configfile,
                   err && err->message ? err->message : _("unknown"));
        goto cleanup;
    }

    if (!(ctx = gvir_sandbox_context_service_new(conn, GVIR_SANDBOX_CONFIG_SERVICE(config)))) {
        g_printerr(_("Unable to create new context service: %s\n"),
                   err && err->message ? err->message : _("unknown"));
        goto cleanup;
    }

cleanup:
    g_free(configfile);
    if (conn)
        g_object_unref(conn);
    if (config)
        g_object_unref(config);

    return ctx ? GVIR_SANDBOX_CONTEXT(ctx) : NULL;
}

static int container_start(const char *uri, const char *name, GMainLoop *loop)
{
    int ret = EXIT_FAILURE;
    GError *err = NULL;
    GVirSandboxConsole *con = NULL;
    GVirSandboxContext *ctx = NULL;

    if (!(ctx = libvirt_sandbox_get_context(uri, name)))
        goto cleanup;

    if (!(gvir_sandbox_context_start(ctx, &err))) {
        g_printerr(_("Unable to start container: %s\n"),
                   err && err->message ? err->message : _("unknown"));
        goto cleanup;
    }

    if (!(con = gvir_sandbox_context_get_log_console(ctx, &err)))  {
        g_printerr(_("Unable to get log console for container: %s\n"),
                   err && err->message ? err->message : _("unknown"));
        goto cleanup;
    }

    gvir_sandbox_console_set_direct(con, TRUE);

    g_signal_connect(con, "closed", (GCallback)do_close, loop);

    if (gvir_sandbox_console_attach_stderr(con, &err) < 0) {
        g_printerr(_("Unable to attach console to stderr in the container: %s\n"),
                   err && err->message ? err->message : _("unknown"));
        goto cleanup;
    }

    /* Stop holding open libvirt connection */
    if (gvir_sandbox_console_isolate(con, &err) < 0) {
        g_printerr(_("Unable to disconnect console from libvirt: %s\n"),
                   err && err->message ? err->message : _("unknown"));
        goto cleanup;
    }

    gvir_sandbox_context_detach(ctx, NULL);
    g_object_unref(ctx);
    ctx = NULL;

    g_main_loop_run(loop);

    ret = EXIT_SUCCESS;

cleanup:
    if (ctx)
        g_object_unref(ctx);
    if (con)
        g_object_unref(con);
    return ret;
}

static int container_attach(const char *uri, const char *name, GMainLoop *loop)
{
    int ret = EXIT_FAILURE;
    GError *err = NULL;
    GVirSandboxConsole *con = NULL;
    GVirSandboxContext *ctx = NULL;

    if (!(ctx = libvirt_sandbox_get_context(uri, name)))
        goto cleanup;

    if (!(gvir_sandbox_context_attach(ctx, &err))) {
        g_printerr(_("Unable to attach to container: %s\n"),
                   err && err->message ? err->message : _("unknown"));
        goto cleanup;
    }

    if (!(con = gvir_sandbox_context_get_shell_console(ctx, &err)))  {
        g_printerr(_("Unable to get shell console for container: %s\n"),
                   err && err->message ? err->message : _("unknown"));
        goto cleanup;
    }

    gvir_sandbox_console_set_direct(con, TRUE);

    g_signal_connect(con, "closed", (GCallback)do_close, loop);

    if (!(gvir_sandbox_console_attach_stdio(con, &err))) {
        g_printerr(_("Unable to attach to container: %s\n"),
                   err && err->message ? err->message : _("unknown"));
        goto cleanup;
    }

    /* Stop holding open libvirt connection */
    if (gvir_sandbox_console_isolate(con, &err) < 0) {
        g_printerr(_("Unable to disconnect console from libvirt: %s\n"),
                   err && err->message ? err->message : _("unknown"));
        goto cleanup;
    }

    gvir_sandbox_context_detach(ctx, NULL);

    g_object_unref(ctx);
    ctx = NULL;

    g_main_loop_run(loop);

    ret = EXIT_SUCCESS;

cleanup:
    if (ctx)
        g_object_unref(ctx);
    if (con)
        g_object_unref(con);
    return ret;
}


static int (*container_func)(const char *uri, const char *name, GMainLoop *loop) = NULL;

static gboolean libvirt_lxc_start(const gchar *option_name,
                                  const gchar *value,
                                  const gpointer *data,
                                  const GError **error)

{
    if (container_func) return FALSE;
    container_func = container_start;
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

int main(int argc, char **argv)
{
    GError *err = NULL;
    GMainLoop *loop = NULL;
    int ret = EXIT_FAILURE;
    pid_t pid = 0;
    gchar *uri = NULL;

    gchar **cmdargs = NULL;
    GOptionContext *context;
    GOptionEntry options[] = {
        { "version", 'V', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
          libvirt_sandbox_version, N_("Display version information"), NULL },
        { "start", 's', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
          libvirt_lxc_start, N_("Start a container"), NULL },
        { "attach", 'a', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
          libvirt_lxc_attach, N_("Attach to a container"), NULL },
        { "pid", 'p', 0, G_OPTION_ARG_INT, &pid,
          N_("Pid of process in container to which the command will run"), "PID"},
        { "connect", 'c', 0, G_OPTION_ARG_STRING, &uri,
          N_("Connect to hypervisor Default:'lxc:///'"), "URI"},
        { G_OPTION_REMAINING, '\0', 0, G_OPTION_ARG_STRING_ARRAY, &cmdargs,
          NULL, "CONTAINER_NAME" },
        { NULL, 0, 0, G_OPTION_ARG_NONE, NULL, NULL, NULL }
    };
    const char *help_msg = N_("Run 'virt-sandbox-service-util --help' to see a full list of available command line options\n");

    setlocale(LC_ALL, "");
    bindtextdomain(PACKAGE, LOCALEDIR);
    bind_textdomain_codeset(PACKAGE, "UTF-8");
    textdomain(PACKAGE);

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

    if ( container_func == NULL ) {
        g_printerr(_("Invalid command: You must specify --start or --attach\n%s"),
                   gettext(help_msg));
        goto cleanup;
    }

    if (!cmdargs || !cmdargs[0] ) {
        g_printerr(_("Invalid command CONTAINER_NAME required: %s"),
                   gettext(help_msg));
        goto cleanup;
    }

    g_option_context_free(context);

    g_set_application_name(_("Libvirt Sandbox Service"));

    loop = g_main_loop_new(g_main_context_default(), 1);
    ret = container_func(uri, cmdargs[0], loop);
    g_main_loop_unref(loop);

cleanup:
    exit(ret);
}
