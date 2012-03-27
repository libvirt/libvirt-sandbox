/*
 * libvirt-sandbox-context.c: libvirt sandbox context
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

#include "libvirt-sandbox/libvirt-sandbox.h"

/**
 * SECTION: libvirt-sandbox-context
 * @short_description: Application sandbox context
 * @include: libvirt-sandbox/libvirt-sandbox.h
 * @see_also: #GVirSandboxContextGraphical
 *
 * Provides a base class for implementing console based application sandboxes
 *
 * The GVirSandboxContext object takes a #GVirSandboxConfig instance, passing it
 * to #GVirSandboxBuilder instance to create a virtual machine, and then provides
 * access to a #GVirSandboxConsole instance for interacting with the sandboxed
 * application's stdio.
 */

#define GVIR_SANDBOX_CONTEXT_GET_PRIVATE(obj)                         \
        (G_TYPE_INSTANCE_GET_PRIVATE((obj), GVIR_SANDBOX_TYPE_CONTEXT, GVirSandboxContextPrivate))

struct _GVirSandboxContextPrivate
{
    GVirConnection *connection;
    GVirDomain *domain;
    GVirSandboxConfig *config;
    GVirSandboxConsole *console;
    gboolean active;

    GVirSandboxBuilder *builder;
    GVirSandboxCleaner *cleaner;
};

G_DEFINE_ABSTRACT_TYPE(GVirSandboxContext, gvir_sandbox_context, G_TYPE_OBJECT);


enum {
    PROP_0,

    PROP_CONFIG,
    PROP_DOMAIN,
    PROP_CONNECTION,
};

enum {
    LAST_SIGNAL
};

//static gint signals[LAST_SIGNAL];

#define GVIR_SANDBOX_CONTEXT_ERROR gvir_sandbox_context_error_quark()

static GQuark
gvir_sandbox_context_error_quark(void)
{
    return g_quark_from_static_string("gvir-sandbox-context");
}

static gboolean gvir_sandbox_context_prestart(GVirSandboxContext *ctxt,
                                              const gchar *configdir,
                                              GError **error);

static void gvir_sandbox_context_get_property(GObject *object,
                                             guint prop_id,
                                             GValue *value,
                                             GParamSpec *pspec)
{
    GVirSandboxContext *ctxt = GVIR_SANDBOX_CONTEXT(object);
    GVirSandboxContextPrivate *priv = ctxt->priv;

    switch (prop_id) {
    case PROP_CONFIG:
        g_value_set_object(value, priv->config);
        break;

    case PROP_DOMAIN:
        g_value_set_object(value, priv->domain);
        break;

    case PROP_CONNECTION:
        g_value_set_object(value, priv->connection);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_context_set_property(GObject *object,
                                             guint prop_id,
                                             const GValue *value,
                                             GParamSpec *pspec)
{
    GVirSandboxContext *ctxt = GVIR_SANDBOX_CONTEXT(object);
    GVirSandboxContextPrivate *priv = ctxt->priv;

    switch (prop_id) {
    case PROP_CONFIG:
        if (priv->config)
            g_object_unref(priv->config);
        priv->config = g_value_dup_object(value);
        break;

    case PROP_DOMAIN:
        if (priv->domain)
            g_object_unref(priv->domain);
        priv->domain = g_value_dup_object(value);
        break;

    case PROP_CONNECTION:
        if (priv->connection)
            g_object_unref(priv->connection);
        priv->connection = g_value_dup_object(value);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_context_finalize(GObject *object)
{
    GVirSandboxContext *ctxt = GVIR_SANDBOX_CONTEXT(object);
    GVirSandboxContextPrivate *priv = ctxt->priv;

    if (priv->active)
        gvir_sandbox_context_stop(ctxt, NULL);

    if (priv->domain)
        g_object_unref(priv->domain);
    if (priv->connection)
        g_object_unref(priv->connection);
    if (priv->config)
        g_object_unref(priv->config);
    if (priv->cleaner)
        g_object_unref(priv->cleaner);

    G_OBJECT_CLASS(gvir_sandbox_context_parent_class)->finalize(object);
}


static void gvir_sandbox_context_class_init(GVirSandboxContextClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = gvir_sandbox_context_finalize;
    object_class->get_property = gvir_sandbox_context_get_property;
    object_class->set_property = gvir_sandbox_context_set_property;

    klass->prestart = gvir_sandbox_context_prestart;

    g_object_class_install_property(object_class,
                                    PROP_CONFIG,
                                    g_param_spec_object("config",
                                                        "Config",
                                                        "The sandbox configuration",
                                                        GVIR_SANDBOX_TYPE_CONFIG,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_DOMAIN,
                                    g_param_spec_object("domain",
                                                        "Domain",
                                                        "The sandbox domain",
                                                        GVIR_TYPE_DOMAIN,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_CONNECTION,
                                    g_param_spec_object("connection",
                                                        "Connection",
                                                        "The sandbox connection",
                                                        GVIR_TYPE_CONNECTION,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_type_class_add_private(klass, sizeof(GVirSandboxContextPrivate));
}


static void gvir_sandbox_context_init(GVirSandboxContext *ctxt)
{
    GVirSandboxContextPrivate *priv = ctxt->priv;
    priv = ctxt->priv = GVIR_SANDBOX_CONTEXT_GET_PRIVATE(ctxt);

    priv->cleaner = gvir_sandbox_cleaner_new();
}


/**
 * gvir_sandbox_context_get_config:
 * @ctxt: (transfer none): the sandbox context
 *
 * Retrieves the sandbox configuration
 *
 * Returns: (transfer full): the current configuration
 */
GVirSandboxConfig *gvir_sandbox_context_get_config(GVirSandboxContext *ctxt)
{
    GVirSandboxContextPrivate *priv = ctxt->priv;
    g_object_ref(priv->config);
    return priv->config;
}


/**
 * gvir_sandbox_context_get_cleaner:
 * @ctxt: (transfer none): the sandbox context
 *
 * Retrieves the sandbox cleaner
 *
 * Returns: (transfer full): the current cleaner
 */
GVirSandboxCleaner *gvir_sandbox_context_get_cleaner(GVirSandboxContext *ctxt)
{
    GVirSandboxContextPrivate *priv = ctxt->priv;
    g_object_ref(priv->cleaner);
    return priv->cleaner;
}


/**
 * gvir_sandbox_context_get_domain:
 * @ctxt: (transfer none): the sandbox context
 *
 * Retrieves the sandbox domain (if running)
 *
 * Returns: (transfer full): the current domain or NULL
 */
GVirDomain *gvir_sandbox_context_get_domain(GVirSandboxContext *ctxt)
{
    GVirSandboxContextPrivate *priv = ctxt->priv;
    if (priv->domain)
        g_object_ref(priv->domain);
    return priv->domain;
}

/**
 * gvir_sandbox_context_get_connection:
 * @ctxt: (transfer none): the sandbox context
 *
 * Retrieves the sandbox connection
 *
 * Returns: (transfer full): the current connection or NULL
 */
GVirConnection *gvir_sandbox_context_get_connection(GVirSandboxContext *ctxt)
{
    GVirSandboxContextPrivate *priv = ctxt->priv;
    if (priv->connection)
        g_object_ref(priv->connection);
    return priv->connection;
}


static gboolean gvir_sandbox_context_prestart(GVirSandboxContext *ctxt,
                                              const gchar *configdir,
                                              GError **error)
{
    GVirSandboxContextPrivate *priv = ctxt->priv;
    gchar *emptydir;
    gchar *configfile;
    gboolean ret = FALSE;

    configfile = g_build_filename(configdir, "sandbox.cfg", NULL);
    emptydir = g_build_filename(configdir, "empty", NULL);

    unlink(configfile);
    if (!gvir_sandbox_config_save_to_path(priv->config, configfile, error))
        goto cleanup;

    gvir_sandbox_cleaner_add_rmfile_post_stop(priv->cleaner,
                                              configfile);

    g_mkdir_with_parents(emptydir, 0755);
    gvir_sandbox_cleaner_add_rmdir_post_stop(priv->cleaner,
                                             emptydir);

    ret = TRUE;
cleanup:
    g_free(configfile);
    g_free(emptydir);
    return ret;
}


gboolean gvir_sandbox_context_start(GVirSandboxContext *ctxt, GError **error)
{
    GVirSandboxContextPrivate *priv = ctxt->priv;
    GVirConfigDomain *config = NULL;
    const gchar *cachedir;
    gchar *tmpdir;
    gchar *configdir;
    gboolean ret = FALSE;

    if (priv->domain) {
        *error = g_error_new(GVIR_SANDBOX_CONTEXT_ERROR, 0,
                             "%s", "A previously built sandbox still exists");
        return FALSE;
    }

    if (!(priv->builder = gvir_sandbox_builder_for_connection(priv->connection,
                                                              error)))
        return FALSE;

    cachedir = g_get_user_cache_dir();
    tmpdir = g_build_filename(cachedir, "libvirt-sandbox",
                              gvir_sandbox_config_get_name(priv->config),
                              NULL);
    configdir = g_build_filename(tmpdir, "config", NULL);

    g_mkdir_with_parents(tmpdir, 0700);
    gvir_sandbox_cleaner_add_rmdir_post_stop(priv->cleaner,
                                             tmpdir);

    g_mkdir_with_parents(configdir, 0700);
    gvir_sandbox_cleaner_add_rmdir_post_stop(priv->cleaner,
                                             configdir);

    if (!GVIR_SANDBOX_CONTEXT_GET_CLASS(ctxt)->prestart(ctxt, configdir, error))
        goto error;

    if (!(config = gvir_sandbox_builder_construct(priv->builder,
                                                  priv->config,
                                                  configdir,
                                                  priv->cleaner,
                                                  error))) {
        goto error;
    }

    if (!(priv->domain = gvir_connection_start_domain(priv->connection,
                                                      config,
                                                      GVIR_DOMAIN_START_AUTODESTROY,
                                                      error)))
        goto error;

    if (!(gvir_sandbox_cleaner_run_post_start(priv->cleaner, NULL)))
        goto error;

    priv->console = gvir_sandbox_console_new(priv->connection, priv->domain,
                                             GVIR_SANDBOX_CONSOLE_TARGET_PRIMARY);

    priv->active = TRUE;
    ret = TRUE;
cleanup:
    g_free(tmpdir);
    g_free(configdir);
    if (config)
        g_object_unref(config);

    return ret;

error:
    if (priv->builder) {
        g_object_unref(priv->builder);
        priv->builder = NULL;
    }
    if (priv->domain) {
        gvir_domain_stop(priv->domain, 0, NULL);
        g_object_unref(priv->domain);
        priv->domain = NULL;
    }

    gvir_sandbox_cleaner_run_post_start(priv->cleaner, NULL);
    gvir_sandbox_cleaner_run_post_stop(priv->cleaner, NULL);

    g_object_unref(priv->cleaner);
    priv->cleaner = gvir_sandbox_cleaner_new();
    goto cleanup;
}


gboolean gvir_sandbox_context_attach(GVirSandboxContext *ctxt, GError **error)
{
    GVirSandboxContextPrivate *priv = ctxt->priv;

    if (priv->domain) {
        *error = g_error_new(GVIR_SANDBOX_CONTEXT_ERROR, 0,
                             "%s", "A previously built sandbox still exists");
        return FALSE;
    }

    if (!gvir_connection_fetch_domains(priv->connection, NULL, error))
        return FALSE;

    if (!(priv->domain = gvir_connection_find_domain_by_name(
              priv->connection,
              gvir_sandbox_config_get_name(priv->config)))) {
        *error = g_error_new(GVIR_SANDBOX_CONTEXT_ERROR, 0,
                             "Sandbox %s does not exist",
                             gvir_sandbox_config_get_name(priv->config));
        return FALSE;
    }

    return TRUE;
}


gboolean gvir_sandbox_context_stop(GVirSandboxContext *ctxt, GError **error)
{
    GVirSandboxContextPrivate *priv = ctxt->priv;
    gboolean ret = TRUE;

    if (priv->domain) {
        if (!gvir_domain_stop(priv->domain, 0, error))
            ret = FALSE;

        g_object_unref(priv->domain);
        priv->domain = NULL;
    }

    if (!gvir_sandbox_cleaner_run_post_stop(priv->cleaner, error))
        ret = FALSE;

    if (priv->builder) {
        g_object_unref(priv->builder);
        priv->builder = NULL;
    }

    g_object_unref(priv->cleaner);
    priv->cleaner = gvir_sandbox_cleaner_new();

    if (priv->console) {
        g_object_unref(priv->console);
        priv->console = NULL;
    }

    priv->active = FALSE;

    return ret;
}


/**
 * gvir_sandbox_context_get_console:
 * @ctxt: (transfer none): the sandbox context
 *
 * Returns: (transfer full)(allow-none): the sandbox console (or NULL)
 */
GVirSandboxConsole *gvir_sandbox_context_get_console(GVirSandboxContext *ctxt)
{
    GVirSandboxContextPrivate *priv = ctxt->priv;

    if (!priv->console)
        return NULL;

    return g_object_ref(priv->console);
}


/**
 * gvir_sandbox_context_get_shell_console:
 * @ctxt: (transfer none): the sandbox context
 *
 * Returns: (transfer full)(allow-none): the sandbox console (or NULL)
 */
GVirSandboxConsole *gvir_sandbox_context_get_shell_console(GVirSandboxContext *ctxt)
{
    GVirSandboxContextPrivate *priv = ctxt->priv;

    if (!priv->domain)
        return NULL;

    return gvir_sandbox_console_new(priv->connection, priv->domain,
                                    GVIR_SANDBOX_CONSOLE_TARGET_SHELL);
}
