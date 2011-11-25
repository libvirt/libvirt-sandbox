#include <libvirt-sandbox/libvirt-sandbox.h>


static gboolean do_close(GVirSandboxConsole *con G_GNUC_UNUSED,
                         gboolean error G_GNUC_UNUSED,
                         gpointer opaque)
{
    GMainLoop *loop = opaque;
    g_main_loop_quit(loop);
    return FALSE;
}

int main(int argc, char **argv) {
    GVirConnection *hv;
    GVirSandboxConfig *cfg;
    GVirSandboxContext *ctx;
    GVirSandboxConsole *con;
    GMainLoop *loop;
    int ret = EXIT_FAILURE;
    GError *error = NULL;

    if (!gvir_init_object_check(&argc, &argv, &error))
        exit(EXIT_FAILURE);

    loop = g_main_loop_new(g_main_context_default(), FALSE);

    hv = gvir_connection_new("qemu:///session");
    if (!gvir_connection_open(hv, NULL, &error))
        exit(EXIT_FAILURE);

    cfg = gvir_sandbox_config_new("sandbox");
    if (argc > 1)
        gvir_sandbox_config_set_command(cfg, argv+1);

    if (isatty(STDIN_FILENO))
        gvir_sandbox_config_set_tty(cfg, TRUE);

    ctx = gvir_sandbox_context_new(hv, cfg);
    if (!gvir_sandbox_context_start(ctx, &error))
        exit(EXIT_FAILURE);

    con = gvir_sandbox_context_get_console(ctx);
    g_signal_connect(con, "closed", (GCallback)do_close, loop);

    if (!(gvir_sandbox_console_attach_stdio(con, &error)))
        goto cleanup;

    g_main_loop_run(loop);

    ret = EXIT_SUCCESS;

cleanup:
    gvir_sandbox_console_detach(con, NULL);
    gvir_sandbox_context_stop(ctx, NULL);
    g_object_unref(ctx);
    g_object_unref(cfg);
    g_main_loop_unref(loop);
    g_object_unref(hv);
    return ret;
}
