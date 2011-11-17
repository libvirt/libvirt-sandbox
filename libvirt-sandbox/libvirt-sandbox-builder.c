/*
 * libvirt-sandbox-builder.c: libvirt sandbox builder
 *
 * Copyright (C) 2011 Red Hat
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 *
 * Author: Daniel P. Berrange <berrange@redhat.com>
 */

#include <config.h>
#include <string.h>

#include "libvirt-sandbox/libvirt-sandbox.h"

#define GVIR_SANDBOX_BUILDER_GET_PRIVATE(obj)                         \
        (G_TYPE_INSTANCE_GET_PRIVATE((obj), GVIR_SANDBOX_TYPE_BUILDER, GVirSandboxBuilderPrivate))

struct _GVirSandboxBuilderPrivate
{
    GVirConnection *connection;
    GVirSandboxConfig *config;
};

G_DEFINE_ABSTRACT_TYPE(GVirSandboxBuilder, gvir_sandbox_builder, G_TYPE_OBJECT);


enum {
    PROP_0,

    PROP_CONFIG,
    PROP_CONNECTION,
};

enum {
    LAST_SIGNAL
};

//static gint signals[LAST_SIGNAL];

#define GVIR_SANDBOX_BUILDER_ERROR gvir_sandbox_builder_error_quark()

static GQuark
gvir_sandbox_builder_error_quark(void)
{
    return g_quark_from_static_string("gvir-sandbox-builder");
}

static void gvir_sandbox_builder_get_property(GObject *object,
                                             guint prop_id,
                                             GValue *value,
                                             GParamSpec *pspec)
{
    GVirSandboxBuilder *ctxt = GVIR_SANDBOX_BUILDER(object);
    GVirSandboxBuilderPrivate *priv = ctxt->priv;

    switch (prop_id) {
    case PROP_CONFIG:
        g_value_set_object(value, priv->config);
        break;

    case PROP_CONNECTION:
        g_value_set_object(value, priv->connection);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_builder_set_property(GObject *object,
                                              guint prop_id,
                                              const GValue *value,
                                              GParamSpec *pspec)
{
    GVirSandboxBuilder *ctxt = GVIR_SANDBOX_BUILDER(object);
    GVirSandboxBuilderPrivate *priv = ctxt->priv;

    switch (prop_id) {
    case PROP_CONFIG:
        if (priv->config)
            g_object_unref(priv->config);
        priv->config = g_value_dup_object(value);
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


static void gvir_sandbox_builder_finalize(GObject *object)
{
    GVirSandboxBuilder *ctxt = GVIR_SANDBOX_BUILDER(object);
    GVirSandboxBuilderPrivate *priv = ctxt->priv;

    if (priv->config)
        g_object_unref(priv->config);
    if (priv->connection)
        g_object_unref(priv->connection);

    G_OBJECT_CLASS(gvir_sandbox_builder_parent_class)->finalize(object);
}


static void gvir_sandbox_builder_class_init(GVirSandboxBuilderClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = gvir_sandbox_builder_finalize;
    object_class->get_property = gvir_sandbox_builder_get_property;
    object_class->set_property = gvir_sandbox_builder_set_property;

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

    g_type_class_add_private(klass, sizeof(GVirSandboxBuilderPrivate));
}


static void gvir_sandbox_builder_init(GVirSandboxBuilder *ctxt)
{
    ctxt->priv = GVIR_SANDBOX_BUILDER_GET_PRIVATE(ctxt);
}


/**
 * gvir_sandbox_builder_for_connection:
 * @connection: (transfer none): the connection to host the sandbox
 * @config: (transfer none): the sandbox configuration
 * @error: (out): the error
 *
 * Find and instantiate a suitable builder for sandboxes to be hosted
 * under the @connection.
 *
 * Returns: (transfer full): a new builder or NULL
 */
GVirSandboxBuilder *gvir_sandbox_builder_for_connection(GVirConnection *connection,
                                                        GVirSandboxConfig *config,
                                                        GError **error)
{
    const gchar *uri = gvir_connection_get_uri(connection);
    GVirSandboxBuilder *builder = NULL;

    if (strcmp(uri, "lxc:///") == 0) {
        builder = GVIR_SANDBOX_BUILDER(gvir_sandbox_builder_container_new(connection, config));
    } else if (strcmp(uri, "qemu:///session") == 0 ||
               strcmp(uri, "qemu:///system") == 0) {
        builder = GVIR_SANDBOX_BUILDER(gvir_sandbox_builder_machine_new(connection, config));
    }

    if (!builder)
        *error = g_error_new(GVIR_SANDBOX_BUILDER_ERROR, 0,
                             "No builder available for URI %s", uri);

    return builder;
}


/**
 * gvir_sandbox_builder_get_config:
 * @builder: (transfer none): the sandbox builder
 *
 * Retrieves the sandbox configuration
 *
 * Returns: (transfer full): the current configuration
 */
GVirSandboxConfig *gvir_sandbox_builder_get_config(GVirSandboxBuilder *builder)
{
    GVirSandboxBuilderPrivate *priv = builder->priv;
    g_object_ref(priv->config);
    return priv->config;
}


/**
 * gvir_sandbox_builder_get_connection:
 * @builder: (transfer none): the sandbox builder
 *
 * Retrieves the sandbox connection
 *
 * Returns: (transfer full): the current connection
 */
GVirConnection *gvir_sandbox_builder_get_connection(GVirSandboxBuilder *builder)
{
    GVirSandboxBuilderPrivate *priv = builder->priv;
    g_object_ref(priv->connection);
    return priv->connection;
}

/**
 * gvir_sandbox_builder_construct:
 * @builder: (transfer none): the sandbox builder
 * @error: (out): the error location
 *
 * Create a domain configuration from the sandbox configuration
 *
 * Returns: (transfer full): the newly built domain configuration
 */
GVirConfigDomain *gvir_sandbox_builder_construct(GVirSandboxBuilder *builder,
                                                 GError **error)
{
    return GVIR_SANDBOX_BUILDER_GET_CLASS(builder)->construct(builder, error);
}


/**
 * gvir_sandbox_builder_cleanup_post_start:
 * @builder: (transfer none): the sandbox builder
 * @error: (out): the error location
 *
 * Remove any temporary files no longer required after the sandbox
 * has started
 *
 * Returns: (transfer full): the newly built domain configuration
 */
gboolean gvir_sandbox_builder_cleanup_post_start(GVirSandboxBuilder *builder G_GNUC_UNUSED,
                                                 GError **error G_GNUC_UNUSED)
{
    return TRUE;
}

/**
 * gvir_sandbox_builder_cleanup_post_start:
 * @builder: (transfer none): the sandbox builder
 * @error: (out): the error location
 *
 * Remove any temporary files no longer required after the sandbox
 * has stopped
 *
 * Returns: (transfer full): the newly built domain configuration
 */
gboolean gvir_sandbox_builder_cleanup_post_stop(GVirSandboxBuilder *builder G_GNUC_UNUSED,
                                                GError **error G_GNUC_UNUSED)
{
    return TRUE;
}
