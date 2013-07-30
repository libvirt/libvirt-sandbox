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

#include <string.h>
#include <errno.h>

#include <glib/gi18n.h>

#include "libvirt-sandbox/libvirt-sandbox.h"

/**
 * SECTION: libvirt-sandbox-context
 * @short_description: Application sandbox context
 * @include: libvirt-sandbox/libvirt-sandbox.h
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
    gboolean active;
    gboolean autodestroy;

    GVirSandboxBuilder *builder;
};

G_DEFINE_ABSTRACT_TYPE(GVirSandboxContext, gvir_sandbox_context, G_TYPE_OBJECT);


enum {
    PROP_0,

    PROP_CONFIG,
    PROP_DOMAIN,
    PROP_CONNECTION,
    PROP_AUTODESTROY,
};

enum {
    LAST_SIGNAL
};

static gboolean gvir_sandbox_context_start_default(GVirSandboxContext *ctxt, GError **error);
static gboolean gvir_sandbox_context_stop_default(GVirSandboxContext *ctxt, GError **error);
static gboolean gvir_sandbox_context_attach_default(GVirSandboxContext *ctxt, GError **error);
static gboolean gvir_sandbox_context_detach_default(GVirSandboxContext *ctxt, GError **error);


//static gint signals[LAST_SIGNAL];

#define GVIR_SANDBOX_CONTEXT_ERROR gvir_sandbox_context_error_quark()

static GQuark
gvir_sandbox_context_error_quark(void)
{
    return g_quark_from_static_string("gvir-sandbox-context");
}

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

    case PROP_AUTODESTROY:
        g_value_set_boolean(value, priv->autodestroy);
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

    case PROP_AUTODESTROY:
        priv->autodestroy = g_value_get_boolean(value);
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

    G_OBJECT_CLASS(gvir_sandbox_context_parent_class)->finalize(object);
}


static void gvir_sandbox_context_class_init(GVirSandboxContextClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = gvir_sandbox_context_finalize;
    object_class->get_property = gvir_sandbox_context_get_property;
    object_class->set_property = gvir_sandbox_context_set_property;

    klass->start = gvir_sandbox_context_start_default;
    klass->stop = gvir_sandbox_context_stop_default;
    klass->attach = gvir_sandbox_context_attach_default;
    klass->detach = gvir_sandbox_context_detach_default;

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
                                                        G_PARAM_WRITABLE |
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

    g_object_class_install_property(object_class,
                                    PROP_AUTODESTROY,
                                    g_param_spec_boolean("autodestroy",
                                                         "Auto destroy",
                                                         "Automatically destroy sandbox on exit",
                                                         TRUE,
                                                         G_PARAM_READABLE |
                                                         G_PARAM_WRITABLE |
                                                         G_PARAM_STATIC_NAME |
                                                         G_PARAM_STATIC_NICK |
                                                         G_PARAM_STATIC_BLURB));

    g_type_class_add_private(klass, sizeof(GVirSandboxContextPrivate));
}


static void gvir_sandbox_context_init(GVirSandboxContext *ctxt)
{
    GVirSandboxContextPrivate *priv;
    priv = ctxt->priv = GVIR_SANDBOX_CONTEXT_GET_PRIVATE(ctxt);

    priv->autodestroy = TRUE;
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
 * gvir_sandbox_context_get_domain:
 * @ctxt: (transfer none): the sandbox context
 *
 * Retrieves the sandbox domain (if running)
 *
 * Returns: (transfer full): the current domain or NULL
 */
GVirDomain *gvir_sandbox_context_get_domain(GVirSandboxContext *ctxt,
                                            GError **error)
{
    GVirSandboxContextPrivate *priv = ctxt->priv;

    if (!priv->domain) {
        g_set_error(error, GVIR_SANDBOX_CONTEXT_ERROR, 0,
                    _("Domain is not currently running"));
        return NULL;
    }

    return g_object_ref(priv->domain);
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


/**
 * gvir_sandbox_context_set_autodestroy:
 * @ctxt: (transfer none): the sandbox context
 * @state: the autodestroy flag state
 *
 * Control whether the sandbox should be automatically
 * destroyed when the client which started it closes its
 * connection to libvirtd. If @state is true, then the
 * sandbox will be automatically destroyed upon client
 * close/exit
 */
void gvir_sandbox_context_set_autodestroy(GVirSandboxContext *ctxt,
                                          gboolean state)
{
    GVirSandboxContextPrivate *priv = ctxt->priv;

    priv->autodestroy = state;
}

/**
 * gvir_sandbox_context_get_autodestroy:
 * @ctxt: (transfer none): the sandbox context
 *
 * Determine whether the sandbox will be automatically
 * destroyed when the client which started it closes its
 * connection to libvirtd
 *
 * Returns: the autodestroy flag state
 */
gboolean gvir_sandbox_context_get_autodestroy(GVirSandboxContext *ctxt)
{
    GVirSandboxContextPrivate *priv = ctxt->priv;

    return priv->autodestroy;
}


static gboolean gvir_sandbox_context_clean_post_start(GVirSandboxContext *ctxt,
                                                      GError **error)
{
    GVirSandboxContextPrivate *priv = ctxt->priv;
    const gchar *cachedir;
    gchar *tmpdir;
    gchar *configdir;
    gboolean ret = TRUE;

    cachedir = (getuid() ? g_get_user_cache_dir() : RUNDIR);
    tmpdir = g_build_filename(cachedir, "libvirt-sandbox",
                              gvir_sandbox_config_get_name(priv->config),
                              NULL);
    configdir = g_build_filename(tmpdir, "config", NULL);

    if (!gvir_sandbox_builder_clean_post_start(priv->builder,
                                               priv->config,
                                               tmpdir,
                                               configdir,
                                               error))
        ret = FALSE;

    g_free(tmpdir);
    g_free(configdir);
    return ret;
}


static gboolean gvir_sandbox_context_clean_post_stop(GVirSandboxContext *ctxt,
                                                     GError **error)
{
    GVirSandboxContextPrivate *priv = ctxt->priv;
    const gchar *cachedir;
    gchar *tmpdir;
    gchar *configdir;
    gchar *configfile;
    gchar *emptydir;
    gboolean ret = TRUE;

    cachedir = (getuid() ? g_get_user_cache_dir() : RUNDIR);
    tmpdir = g_build_filename(cachedir, "libvirt-sandbox",
                              gvir_sandbox_config_get_name(priv->config),
                              NULL);
    configdir = g_build_filename(tmpdir, "config", NULL);
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

    if (rmdir(tmpdir) < 0 &&
        errno != ENOENT)
        ret = FALSE;

    if (!gvir_sandbox_builder_clean_post_stop(priv->builder,
                                              priv->config,
                                              tmpdir,
                                              configdir,
                                              error))
        ret = FALSE;

    g_free(configfile);
    g_free(emptydir);
    g_free(tmpdir);
    g_free(configdir);
    return ret;
}


static gboolean gvir_sandbox_context_start_default(GVirSandboxContext *ctxt, GError **error)
{
    GVirSandboxContextPrivate *priv = ctxt->priv;
    GVirConfigDomain *config = NULL;
    const gchar *cachedir;
    gchar *tmpdir;
    gchar *configdir;
    gchar *emptydir;
    gchar *configfile;
    gboolean ret = FALSE;
    int flags = 0;

    if (priv->domain) {
        *error = g_error_new(GVIR_SANDBOX_CONTEXT_ERROR, 0,
                             "%s", "A previously built sandbox still exists");
        return FALSE;
    }

    if (!(priv->builder = gvir_sandbox_builder_for_connection(priv->connection,
                                                              error)))
        return FALSE;

    cachedir = (getuid() ? g_get_user_cache_dir() : RUNDIR);
    tmpdir = g_build_filename(cachedir, "libvirt-sandbox",
                              gvir_sandbox_config_get_name(priv->config),
                              NULL);
    configdir = g_build_filename(tmpdir, "config", NULL);
    configfile = g_build_filename(configdir, "sandbox.cfg", NULL);
    emptydir = g_build_filename(configdir, "empty", NULL);

    g_mkdir_with_parents(tmpdir, 0700);
    g_mkdir_with_parents(configdir, 0700);

    unlink(configfile);
    if (!gvir_sandbox_config_save_to_path(priv->config, configfile, error))
        goto error;

    g_mkdir_with_parents(emptydir, 0755);

    if (!(config = gvir_sandbox_builder_construct(priv->builder,
                                                  priv->config,
                                                  tmpdir,
                                                  configdir,
                                                  error))) {
        goto error;
    }

    if (priv->autodestroy)
        flags |= GVIR_DOMAIN_START_AUTODESTROY;

    if (!(priv->domain = gvir_connection_start_domain(priv->connection,
                                                      config,
                                                      flags,
                                                      error)))
        goto error;

    if (!gvir_sandbox_context_clean_post_start(ctxt, error))
        goto error;

    priv->active = TRUE;
    ret = TRUE;
cleanup:
    g_free(tmpdir);
    g_free(configdir);
    g_free(configfile);
    g_free(emptydir);
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

    goto cleanup;
}


static gboolean gvir_sandbox_context_attach_default(GVirSandboxContext *ctxt, GError **error)
{
    GVirSandboxContextPrivate *priv = ctxt->priv;

    if (priv->active) {
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

    if (!(priv->builder = gvir_sandbox_builder_for_connection(priv->connection,
                                                              error)))
        return FALSE;

    priv->active = TRUE;

    return TRUE;
}


static gboolean gvir_sandbox_context_detach_default(GVirSandboxContext *ctxt, GError **error)
{
    GVirSandboxContextPrivate *priv = ctxt->priv;

    if (!priv->active)
        return TRUE;

    if (priv->domain) {
        g_object_unref(priv->domain);
        priv->domain = NULL;
    }
    if (priv->builder) {
        g_object_unref(priv->builder);
        priv->builder = NULL;
    }

    priv->active = FALSE;

    return TRUE;
}


static gboolean gvir_sandbox_context_stop_default(GVirSandboxContext *ctxt, GError **error)
{
    GVirSandboxContextPrivate *priv = ctxt->priv;
    gboolean ret = TRUE;

    if (!priv->active)
        return TRUE;

    if (priv->domain) {
        if (!gvir_domain_stop(priv->domain, 0, error))
            ret = FALSE;

        g_object_unref(priv->domain);
        priv->domain = NULL;
    }

    if (!gvir_sandbox_context_clean_post_stop(ctxt, error))
        ret = FALSE;

    if (priv->builder) {
        g_object_unref(priv->builder);
        priv->builder = NULL;
    }

    priv->active = FALSE;

    return ret;
}


/**
 * gvir_sandbox_context_get_log_console:
 * @ctxt: (transfer none): the sandbox context
 *
 * Returns: (transfer full)(allow-none): the sandbox console (or NULL)
 */
GVirSandboxConsole *gvir_sandbox_context_get_log_console(GVirSandboxContext *ctxt,
                                                         GError **error)
{
    GVirSandboxContextPrivate *priv = ctxt->priv;
    GVirSandboxConsole *console;
    const char *devname = NULL;

    if (!priv->domain) {
        g_set_error(error, GVIR_SANDBOX_CONTEXT_ERROR, 0,
                    _("Domain is not currently running"));
        return NULL;
    }

    /* XXX get from config */
    if (strstr(gvir_connection_get_uri(priv->connection), "lxc"))
        devname = "console0";
    else
        devname = "serial0";

    console = GVIR_SANDBOX_CONSOLE(gvir_sandbox_console_raw_new(priv->connection,
                                                                priv->domain,
                                                                devname));

    return console;
}


/**
 * gvir_sandbox_context_get_shell_console:
 * @ctxt: (transfer none): the sandbox context
 *
 * Returns: (transfer full)(allow-none): the sandbox console (or NULL)
 */
GVirSandboxConsole *gvir_sandbox_context_get_shell_console(GVirSandboxContext *ctxt,
                                                           GError **error)
{
    GVirSandboxContextPrivate *priv = ctxt->priv;
    GVirSandboxConsole *console;
    const char *devname = NULL;

    if (!priv->domain) {
        g_set_error(error, GVIR_SANDBOX_CONTEXT_ERROR, 0,
                    _("Domain is not currently running"));
        return NULL;
    }

    /* XXX get from config */
    if (strstr(gvir_connection_get_uri(priv->connection), "lxc"))
        devname = "console1";
    else
        devname = "serial1";

    console = GVIR_SANDBOX_CONSOLE(gvir_sandbox_console_raw_new(priv->connection, priv->domain,
                                                                devname));
    return console;
}

gboolean gvir_sandbox_context_start(GVirSandboxContext *ctxt, GError **error)
{
    return GVIR_SANDBOX_CONTEXT_GET_CLASS(ctxt)->start(ctxt, error);
}


gboolean gvir_sandbox_context_stop(GVirSandboxContext *ctxt, GError **error)
{
    return GVIR_SANDBOX_CONTEXT_GET_CLASS(ctxt)->stop(ctxt, error);
}


gboolean gvir_sandbox_context_attach(GVirSandboxContext *ctxt, GError **error)
{
    return GVIR_SANDBOX_CONTEXT_GET_CLASS(ctxt)->attach(ctxt, error);
}


gboolean gvir_sandbox_context_detach(GVirSandboxContext *ctxt, GError **error)
{
    return GVIR_SANDBOX_CONTEXT_GET_CLASS(ctxt)->detach(ctxt, error);
}
