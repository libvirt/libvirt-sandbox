/*
 * libvirt-sandbox-context-interactive.c: libvirt sandbox interactive context
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
#include <string.h>
#include <errno.h>

#include "libvirt-sandbox/libvirt-sandbox.h"

/**
 * SECTION: libvirt-sandbox-context-interactive
 * @short_description: Desktop application sandbox context
 * @include: libvirt-sandbox/libvirt-sandbox.h
 * @see_also: #GVirSandboxContext
 *
 * Provides a base class for implementing interactive desktop application sandboxes
 *
 * The GVirSandboxContextInteractive object extends the functionality provided by
 * #GVirSandboxContext to allow the application to display output in a interactive
 * desktop.
 */

#define GVIR_SANDBOX_CONTEXT_INTERACTIVE_GET_PRIVATE(obj)                         \
        (G_TYPE_INSTANCE_GET_PRIVATE((obj), GVIR_SANDBOX_TYPE_CONTEXT_INTERACTIVE, GVirSandboxContextInteractivePrivate))

struct _GVirSandboxContextInteractivePrivate
{
    gboolean unused;
};

G_DEFINE_TYPE(GVirSandboxContextInteractive, gvir_sandbox_context_interactive, GVIR_SANDBOX_TYPE_CONTEXT);


enum {
    PROP_0,
};

enum {
    LAST_SIGNAL
};

//static gint signals[LAST_SIGNAL];


static void gvir_sandbox_context_interactive_get_property(GObject *object,
                                                          guint prop_id,
                                                          GValue *value G_GNUC_UNUSED,
                                                          GParamSpec *pspec)
{
#if 0
    GVirSandboxContextInteractive *context = GVIR_SANDBOX_CONTEXT_INTERACTIVE(object);
    GVirSandboxContextInteractivePrivate *priv = context->priv;
#endif

    switch (prop_id) {

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_context_interactive_set_property(GObject *object,
                                                          guint prop_id,
                                                          const GValue *value G_GNUC_UNUSED,
                                                          GParamSpec *pspec)
{
#if 0
    GVirSandboxContextInteractive *context = GVIR_SANDBOX_CONTEXT_INTERACTIVE(object);
    GVirSandboxContextInteractivePrivate *priv = context->priv;
#endif

    switch (prop_id) {

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_context_interactive_finalize(GObject *object)
{
#if 0
    GVirSandboxContextInteractive *context = GVIR_SANDBOX_CONTEXT_INTERACTIVE(object);
    GVirSandboxContextInteractivePrivate *priv = context->priv;
#endif

    G_OBJECT_CLASS(gvir_sandbox_context_interactive_parent_class)->finalize(object);
}


static gboolean gvir_sandbox_context_clean_post_start(GVirSandboxContext *ctxt,
                                                      GVirSandboxBuilder *builder,
                                                      GError **error)
{
    const gchar *cachedir;
    gchar *tmpdir;
    gboolean ret = TRUE;
    GVirSandboxConfig *config = gvir_sandbox_context_get_config(ctxt);

    cachedir = (getuid() ? g_get_user_cache_dir() : RUNDIR);
    tmpdir = g_build_filename(cachedir, "libvirt-sandbox",
                              gvir_sandbox_config_get_name(config),
                              NULL);

    if (!gvir_sandbox_builder_clean_post_start(builder,
                                               config,
                                               tmpdir,
                                               error))
        ret = FALSE;

    g_free(tmpdir);
    g_object_unref(config);
    return ret;
}


static gboolean gvir_sandbox_context_clean_post_stop(GVirSandboxContext *ctxt,
                                                     GVirSandboxBuilder *builder,
                                                     GError **error)
{
    const gchar *cachedir;
    gchar *statedir;
    gchar *configdir;
    gchar *configfile;
    gchar *emptydir;
    gboolean ret = TRUE;
    GVirSandboxConfig *config = gvir_sandbox_context_get_config(ctxt);

    cachedir = (getuid() ? g_get_user_cache_dir() : RUNDIR);
    statedir = g_build_filename(cachedir, "libvirt-sandbox",
                                gvir_sandbox_config_get_name(config),
                                NULL);
    configdir = g_build_filename(statedir, "config", NULL);
    configfile = g_build_filename(configdir, "sandbox.cfg", NULL);
    emptydir = g_build_filename(configdir, "empty", NULL);

    if (unlink(configfile) < 0 &&
        errno != ENOENT)
        ret = FALSE;

    if (rmdir(emptydir) < 0 &&
        errno != ENOENT)
        ret = FALSE;

    if (rmdir(configdir) < 0 &&
        errno != ENOENT)
        ret = FALSE;

    if (rmdir(statedir) < 0 &&
        errno != ENOENT)
        ret = FALSE;

    if (!gvir_sandbox_builder_clean_post_stop(builder,
                                              config,
                                              statedir,
                                              error))
        ret = FALSE;

    g_object_unref(config);
    g_free(configfile);
    g_free(emptydir);
    g_free(statedir);
    g_free(configdir);
    return ret;
}


static gboolean gvir_sandbox_context_interactive_start(GVirSandboxContext *ctxt, GError **error)
{
    GVirConfigDomain *configdom = NULL;
    GVirSandboxBuilder *builder = NULL;
    GVirConnection *connection = NULL;
    GVirDomain *domain = NULL;
    GVirSandboxConfig *config = NULL;
    const gchar *cachedir;
    gchar *statedir;
    gchar *configdir;
    gchar *emptydir;
    gchar *configfile;
    gboolean ret = FALSE;

    if (!GVIR_SANDBOX_CONTEXT_CLASS(gvir_sandbox_context_interactive_parent_class)->start(ctxt, error))
        return FALSE;

    connection = gvir_sandbox_context_get_connection(ctxt);
    config = gvir_sandbox_context_get_config(ctxt);

    cachedir = (getuid() ? g_get_user_cache_dir() : RUNDIR);
    statedir = g_build_filename(cachedir, "libvirt-sandbox",
                              gvir_sandbox_config_get_name(config),
                              NULL);
    configdir = g_build_filename(statedir, "config", NULL);
    configfile = g_build_filename(configdir, "sandbox.cfg", NULL);
    emptydir = g_build_filename(configdir, "empty", NULL);

    if (!(builder = gvir_sandbox_builder_for_connection(connection,
                                                        error)))
        goto cleanup;

    g_mkdir_with_parents(statedir, 0700);
    g_mkdir_with_parents(configdir, 0700);

    unlink(configfile);
    if (!gvir_sandbox_config_save_to_path(config, configfile, error))
        goto cleanup;

    g_mkdir_with_parents(emptydir, 0755);

    if (!(configdom = gvir_sandbox_builder_construct(builder,
                                                     config,
                                                     statedir,
                                                     error))) {
        goto cleanup;
    }

    if (!(domain = gvir_connection_start_domain(connection,
                                                configdom,
                                                GVIR_DOMAIN_START_AUTODESTROY,
                                                error)))
        goto cleanup;

    if (!gvir_sandbox_context_clean_post_start(ctxt, builder, error))
        goto cleanup;

    g_object_set(ctxt, "domain", domain, NULL);

    ret = TRUE;
cleanup:
    if (!ret && domain)
        gvir_domain_stop(domain, 0, NULL);
    g_free(statedir);
    g_free(configdir);
    g_free(configfile);
    g_free(emptydir);
    if (configdom)
        g_object_unref(configdom);
    if (builder)
        g_object_unref(builder);
    g_object_unref(config);
    g_object_unref(connection);
    if (domain)
        g_object_unref(domain);

    return ret;
}

static gboolean gvir_sandbox_context_interactive_stop(GVirSandboxContext *ctxt, GError **error)
{
    GVirDomain *domain;
    GVirConnection *connection;
    GVirSandboxBuilder *builder;
    gboolean ret = TRUE;

    if (!GVIR_SANDBOX_CONTEXT_CLASS(gvir_sandbox_context_interactive_parent_class)->stop(ctxt, error))
        return FALSE;


    if (!(domain = gvir_sandbox_context_get_domain(ctxt, error)))
        return FALSE;

    if (!gvir_domain_stop(domain, 0, error))
        ret = FALSE;

    g_object_set(ctxt, "domain", NULL, NULL);
    g_object_unref(domain);

    connection = gvir_sandbox_context_get_connection(ctxt);

    if (!(builder = gvir_sandbox_builder_for_connection(connection,
                                                        error)))
        ret = FALSE;

    if (builder &&
        !gvir_sandbox_context_clean_post_stop(ctxt, builder, error))
        ret = FALSE;

    if (builder)
        g_object_unref(builder);
    g_object_unref(connection);

    return ret;
}


static void gvir_sandbox_context_interactive_class_init(GVirSandboxContextInteractiveClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    GVirSandboxContextClass *context_class = GVIR_SANDBOX_CONTEXT_CLASS(klass);

    object_class->finalize = gvir_sandbox_context_interactive_finalize;
    object_class->get_property = gvir_sandbox_context_interactive_get_property;
    object_class->set_property = gvir_sandbox_context_interactive_set_property;

    context_class->start = gvir_sandbox_context_interactive_start;
    context_class->stop = gvir_sandbox_context_interactive_stop;

    g_type_class_add_private(klass, sizeof(GVirSandboxContextInteractivePrivate));
}


static void gvir_sandbox_context_interactive_init(GVirSandboxContextInteractive *context)
{
    context->priv = GVIR_SANDBOX_CONTEXT_INTERACTIVE_GET_PRIVATE(context);
}


/**
 * gvir_sandbox_context_interactive_new:
 * @connection: (transfer none): the libvirt connection
 * @config: (transfer none): the initial configuratoion
 *
 * Create a new interactive application sandbox context
 *
 * Returns: (transfer full): a new interactive sandbox context object
 */
GVirSandboxContextInteractive *gvir_sandbox_context_interactive_new(GVirConnection *connection,
                                                                    GVirSandboxConfigInteractive *config)
{
    return GVIR_SANDBOX_CONTEXT_INTERACTIVE(g_object_new(GVIR_SANDBOX_TYPE_CONTEXT_INTERACTIVE,
                                                         "connection", connection,
                                                         "config", config,
                                                         NULL));
}


/**
 * gvir_sandbox_context_interactive_get_app_console:
 * @ctxt: (transfer none): the sandbox context
 *
 * Returns: (transfer full)(allow-none): the sandbox console (or NULL)
 */
GVirSandboxConsole *gvir_sandbox_context_interactive_get_app_console(GVirSandboxContextInteractive *ctxt,
                                                                     GError **error)
{
    GVirSandboxConsole *console;
    const char *devname = NULL;
    GVirDomain *domain;
    GVirConnection *conn;

    if (!(domain = gvir_sandbox_context_get_domain(GVIR_SANDBOX_CONTEXT(ctxt), error)))
        return NULL;

    conn = gvir_sandbox_context_get_connection(GVIR_SANDBOX_CONTEXT(ctxt));

    /* XXX get from config + shell */
    if (strstr(gvir_connection_get_uri(conn), "lxc")) {
        GVirSandboxConfig *config = gvir_sandbox_context_get_config(GVIR_SANDBOX_CONTEXT(ctxt));
        if (gvir_sandbox_config_get_shell(config))
            devname = "console2";
        else
            devname = "console1";
    } else {
        devname = "console1";
    }

    console = GVIR_SANDBOX_CONSOLE(gvir_sandbox_console_rpc_new(conn,
                                                                domain,
                                                                devname));
    g_object_unref(domain);
    return console;
}
