/*
 * libvirt-sandbox-config-graphical.c: libvirt sandbox configuration
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

/**
 * SECTION: libvirt-sandbox-config-graphical
 * @short_description: Graphical sandbox configuration details
 * @include: libvirt-sandbox/libvirt-sandbox.h
 * @see_also: #GVirSandboxConfigGraphical
 *
 * Provides an object to store configuration details for a graphical config
 *
 * The GVirSandboxConfigGraphical object extends #GVirSandboxConfig to store
 * the extra information required to setup a graphical desktop application
 * sandbox.
 */

#define GVIR_SANDBOX_CONFIG_GRAPHICAL_GET_PRIVATE(obj)                         \
        (G_TYPE_INSTANCE_GET_PRIVATE((obj), GVIR_SANDBOX_TYPE_CONFIG_GRAPHICAL, GVirSandboxConfigGraphicalPrivate))

struct _GVirSandboxConfigGraphicalPrivate
{
    gchar *windowManager;
    GVirSandboxConfigGraphicalSize windowSize;
};

G_DEFINE_TYPE(GVirSandboxConfigGraphical, gvir_sandbox_config_graphical, GVIR_SANDBOX_TYPE_CONFIG);


enum {
    PROP_0,
    PROP_WINDOW_MANAGER,
    PROP_WINDOW_SIZE
};

enum {
    LAST_SIGNAL
};

//static gint signals[LAST_SIGNAL];


static GVirSandboxConfigGraphicalSize *
gvir_sandbox_config_graphical_size_copy(GVirSandboxConfigGraphicalSize *size)
{
    GVirSandboxConfigGraphicalSize *ret = g_new0(GVirSandboxConfigGraphicalSize, 1);
    memcpy(ret, size, sizeof(*ret));
    return ret;
}

static void
gvir_sandbox_config_graphical_size_free(GVirSandboxConfigGraphicalSize *size)
{
    g_free(size);
}


GType gvir_sandbox_config_graphical_size_get_type(void)
{
    static GType size_type = 0;

    if (G_UNLIKELY(size_type == 0))
        size_type = g_boxed_type_register_static
            ("GVirSandboxConfigGraphicalSize",
             (GBoxedCopyFunc)gvir_sandbox_config_graphical_size_copy,
             (GBoxedFreeFunc)gvir_sandbox_config_graphical_size_free);

    return size_type;
}

static void gvir_sandbox_config_graphical_get_property(GObject *object,
                                                       guint prop_id,
                                                       GValue *value,
                                                       GParamSpec *pspec)
{
    GVirSandboxConfigGraphical *config = GVIR_SANDBOX_CONFIG_GRAPHICAL(object);
    GVirSandboxConfigGraphicalPrivate *priv = config->priv;

    switch (prop_id) {
    case PROP_WINDOW_MANAGER:
        g_value_set_string(value, priv->windowManager);
        break;

    case PROP_WINDOW_SIZE:
        g_value_set_boxed(value, &priv->windowSize);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_config_graphical_set_property(GObject *object,
                                                       guint prop_id,
                                                       const GValue *value,
                                                       GParamSpec *pspec)
{
    GVirSandboxConfigGraphical *config = GVIR_SANDBOX_CONFIG_GRAPHICAL(object);
    GVirSandboxConfigGraphicalPrivate *priv = config->priv;

    switch (prop_id) {
    case PROP_WINDOW_MANAGER:
        g_free(priv->windowManager);
        priv->windowManager = g_value_dup_string(value);
        break;

    case PROP_WINDOW_SIZE:
        memcpy(&priv->windowSize, g_value_get_boxed(value), sizeof(priv->windowSize));
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_config_graphical_finalize(GObject *object)
{
    GVirSandboxConfigGraphical *config = GVIR_SANDBOX_CONFIG_GRAPHICAL(object);
    GVirSandboxConfigGraphicalPrivate *priv = config->priv;

    g_free(priv->windowManager);

    G_OBJECT_CLASS(gvir_sandbox_config_graphical_parent_class)->finalize(object);
}


static void gvir_sandbox_config_graphical_class_init(GVirSandboxConfigGraphicalClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = gvir_sandbox_config_graphical_finalize;
    object_class->get_property = gvir_sandbox_config_graphical_get_property;
    object_class->set_property = gvir_sandbox_config_graphical_set_property;

    g_object_class_install_property(object_class,
                                    PROP_WINDOW_MANAGER,
                                    g_param_spec_string("window-manager",
                                                        "Window manager",
                                                        "The path to the window manager binary",
                                                        NULL,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_WINDOW_SIZE,
                                    g_param_spec_boxed("window-size",
                                                       "Window size",
                                                       "The dimensions of the window",
                                                       GVIR_SANDBOX_TYPE_CONFIG_GRAPHICAL_SIZE,
                                                       G_PARAM_READABLE |
                                                       G_PARAM_WRITABLE |
                                                       G_PARAM_STATIC_NAME |
                                                       G_PARAM_STATIC_NICK |
                                                       G_PARAM_STATIC_BLURB));

    g_type_class_add_private(klass, sizeof(GVirSandboxConfigGraphicalPrivate));
}


static void gvir_sandbox_config_graphical_init(GVirSandboxConfigGraphical *config)
{
    GVirSandboxConfigGraphicalPrivate *priv;

    priv = config->priv = GVIR_SANDBOX_CONFIG_GRAPHICAL_GET_PRIVATE(config);

    priv->windowManager = g_strdup(BINDIR "/matchbox-window-manager");
    priv->windowSize.width = 1024;
    priv->windowSize.height = 768;
}


/**
 * gvir_sandbox_config_graphical_new:
 *
 * Create a new graphical application sandbox configuration
 *
 * Returns: (transfer full): a new graphical sandbox config object
 */
GVirSandboxConfigGraphical *gvir_sandbox_config_graphical_new(void)
{
    return GVIR_SANDBOX_CONFIG_GRAPHICAL(g_object_new(GVIR_SANDBOX_TYPE_CONFIG_GRAPHICAL,
                                                      NULL));
}


/**
 * gvir_sandbox_config_set_window_manager:
 * @config: (transfer none): the sandbox config
 * @hostpath: (transfer none): the host binary path
 *
 * Set the host binary to use as the window manager within the sandbox
 * The default binary is "/usr/bin/matchbox-window-manager"
 */
void gvir_sandbox_config_graphical_set_window_manager(GVirSandboxConfigGraphical *config, const gchar *hostpath)
{
    GVirSandboxConfigGraphicalPrivate *priv = config->priv;
    g_free(priv->windowManager);
    priv->windowManager = g_strdup(hostpath);
}


/**
 * gvir_sandbox_config_get_window_manager:
 * @config: (transfer none): the sandbox config
 *
 * Retrieves the binary used as the window manager in the sandbox
 *
 * Returns: (transfer none): the binary path
 */
const gchar *gvir_sandbox_config_graphical_get_window_manager(GVirSandboxConfigGraphical *config)
{
    GVirSandboxConfigGraphicalPrivate *priv = config->priv;
    return priv->windowManager;
}


/**
 * gvir_sandbox_config_set_window_size:
 * @config: (transfer none): the sandbox config
 * @size: (transfer none): the window size
 *
 * Sets the desired initial size for the sandbox graphical window
 */
void gvir_sandbox_config_graphical_set_window_size(GVirSandboxConfigGraphical *config, GVirSandboxConfigGraphicalSize *size)
{
    GVirSandboxConfigGraphicalPrivate *priv = config->priv;
    memcpy(&priv->windowSize, size, sizeof(*size));
}


/**
 * gvir_sandbox_config_set_window_size:
 * @config: (transfer none): the sandbox config
 *
 * Retrieves the graphical window size
 *
 * Returns: (transfer none): the window size
 */
GVirSandboxConfigGraphicalSize *gvir_sandbox_config_graphical_get_window_size(GVirSandboxConfigGraphical *config)
{
    GVirSandboxConfigGraphicalPrivate *priv = config->priv;
    return &priv->windowSize;
}
