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

    if (priv->domain)
        gvir_sandbox_context_stop(ctxt, NULL);

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

    g_type_class_add_private(klass, sizeof(GVirSandboxContextPrivate));
}


static void gvir_sandbox_context_init(GVirSandboxContext *ctxt)
{
    ctxt->priv = GVIR_SANDBOX_CONTEXT_GET_PRIVATE(ctxt);
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


static gboolean gvir_sandbox_context_start_default(GVirSandboxContext *ctxt, GError **error)
{
    GVirSandboxContextPrivate *priv = ctxt->priv;

    if (priv->domain) {
        *error = g_error_new(GVIR_SANDBOX_CONTEXT_ERROR, 0,
                             "%s", "A previously built sandbox still exists");
        return FALSE;
    }

    return TRUE;
}


static gboolean gvir_sandbox_context_attach_default(GVirSandboxContext *ctxt, GError **error)
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


static gboolean gvir_sandbox_context_detach_default(GVirSandboxContext *ctxt, GError **error)
{
    GVirSandboxContextPrivate *priv = ctxt->priv;

    if (!priv->domain) {
        g_set_error(error, GVIR_SANDBOX_CONTEXT_ERROR, 0,
                    _("Sandbox is not currently running"));
        return FALSE;
    }

    g_object_unref(priv->domain);
    priv->domain = NULL;

    return TRUE;
}


static gboolean gvir_sandbox_context_stop_default(GVirSandboxContext *ctxt, GError **error)
{
    GVirSandboxContextPrivate *priv = ctxt->priv;

    if (!priv->domain) {
        g_set_error(error, GVIR_SANDBOX_CONTEXT_ERROR, 0,
                    _("Sandbox is not currently running"));
        return FALSE;
    }

    return TRUE;
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
                    _("Sandbox is not currently running"));
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

gboolean gvir_sandbox_context_is_attached(GVirSandboxContext *ctxt)
{
    GVirSandboxContextPrivate *priv = ctxt->priv;
    return priv->domain != NULL;
}
