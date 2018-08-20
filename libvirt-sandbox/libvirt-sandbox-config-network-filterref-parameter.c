/*
 * libvirt-sandbox-config-network-filterref-parameter.c: libvirt sandbox configuration
 *
 * Copyright (C) 2014 Red Hat, Inc.
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
 * Author: Ian Main <imain@redhat.com>
 */

#include <config.h>
#include <string.h>

#include "libvirt-sandbox/libvirt-sandbox-config-all.h"

/**
 * SECTION: libvirt-sandbox-config-network-filterref-parameter
 * @short_description: Set parameters for a filter reference.
 * @include: libvirt-sandbox/libvirt-sandbox.h
 *
 * Provides an object to store filter parameter name and value.
 *
 * The GVirSandboxConfigNetworkFilterrefParameter object stores a
 * name and value required to set a single parameter of a filter reference.
 */

#define GVIR_SANDBOX_CONFIG_NETWORK_FILTERREF_PARAMETER_GET_PRIVATE(obj)                         \
        (G_TYPE_INSTANCE_GET_PRIVATE((obj), GVIR_SANDBOX_TYPE_CONFIG_NETWORK_FILTERREF_PARAMETER, GVirSandboxConfigNetworkFilterrefParameterPrivate))

struct _GVirSandboxConfigNetworkFilterrefParameterPrivate
{
    gchar *name;
    gchar *value;
};

G_DEFINE_TYPE_WITH_PRIVATE(GVirSandboxConfigNetworkFilterrefParameter, gvir_sandbox_config_network_filterref_parameter, G_TYPE_OBJECT);


enum {
    PROP_0,
    PROP_NAME,
    PROP_VALUE,
};

enum {
    LAST_SIGNAL
};

//static gint signals[LAST_SIGNAL];


static void gvir_sandbox_config_network_filterref_parameter_get_property(GObject *object,
                                                                         guint prop_id,
                                                                         GValue *value,
                                                                         GParamSpec *pspec)
{
    GVirSandboxConfigNetworkFilterrefParameter *config = GVIR_SANDBOX_CONFIG_NETWORK_FILTERREF_PARAMETER(object);
    GVirSandboxConfigNetworkFilterrefParameterPrivate *priv = config->priv;

    switch (prop_id) {
    case PROP_NAME:
        g_value_set_string(value, priv->name);
        break;

    case PROP_VALUE:
        g_value_set_string(value, priv->value);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_config_network_filterref_parameter_set_property(GObject *object,
                                                                         guint prop_id,
                                                                         const GValue *value,
                                                                         GParamSpec *pspec)
{
    GVirSandboxConfigNetworkFilterrefParameter *filter = GVIR_SANDBOX_CONFIG_NETWORK_FILTERREF_PARAMETER(object);

    switch (prop_id) {
    case PROP_NAME:
        gvir_sandbox_config_network_filterref_parameter_set_name(filter, g_value_get_string(value));
        break;

    case PROP_VALUE:
        gvir_sandbox_config_network_filterref_parameter_set_value(filter, g_value_get_string(value));
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_config_network_filterref_parameter_finalize(GObject *object)
{
    GVirSandboxConfigNetworkFilterrefParameter *config = GVIR_SANDBOX_CONFIG_NETWORK_FILTERREF_PARAMETER(object);
    GVirSandboxConfigNetworkFilterrefParameterPrivate *priv = config->priv;

    g_free(priv->name);
    g_free(priv->value);

    G_OBJECT_CLASS(gvir_sandbox_config_network_filterref_parameter_parent_class)->finalize(object);
}


static void gvir_sandbox_config_network_filterref_parameter_class_init(GVirSandboxConfigNetworkFilterrefParameterClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = gvir_sandbox_config_network_filterref_parameter_finalize;
    object_class->get_property = gvir_sandbox_config_network_filterref_parameter_get_property;
    object_class->set_property = gvir_sandbox_config_network_filterref_parameter_set_property;

    g_object_class_install_property(object_class,
                                    PROP_NAME,
                                    g_param_spec_string("name",
                                                        "Name",
                                                        "Name of parameter",
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_STRINGS));

    g_object_class_install_property(object_class,
                                    PROP_VALUE,
                                    g_param_spec_string("value",
                                                        "Value",
                                                        "Value of parameter",
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_STRINGS));
}


static void gvir_sandbox_config_network_filterref_parameter_init(GVirSandboxConfigNetworkFilterrefParameter *param)
{
    param->priv = GVIR_SANDBOX_CONFIG_NETWORK_FILTERREF_PARAMETER_GET_PRIVATE(param);
}


/**
 * gvir_sandbox_config_network_filterref_parameter_new:
 *
 * Create a new network config with DHCP enabled
 *
 * Returns: (transfer full): a new sandbox network object
 */
GVirSandboxConfigNetworkFilterrefParameter *gvir_sandbox_config_network_filterref_parameter_new(void)
{
    return GVIR_SANDBOX_CONFIG_NETWORK_FILTERREF_PARAMETER(g_object_new(GVIR_SANDBOX_TYPE_CONFIG_NETWORK_FILTERREF_PARAMETER,
                                                                        NULL));
}

void gvir_sandbox_config_network_filterref_parameter_set_name(GVirSandboxConfigNetworkFilterrefParameter *param,
                                                              const gchar *name)
{
    g_return_if_fail(GVIR_SANDBOX_IS_CONFIG_NETWORK_FILTERREF_PARAMETER(param));

    g_free(param->priv->name);
    param->priv->name = g_strdup(name);
    g_object_notify(G_OBJECT(param), "name");
}

const gchar *gvir_sandbox_config_network_filterref_parameter_get_name(GVirSandboxConfigNetworkFilterrefParameter *param)
{
    g_return_val_if_fail(GVIR_SANDBOX_IS_CONFIG_NETWORK_FILTERREF_PARAMETER(param), NULL);

    return param->priv->name;
}

void gvir_sandbox_config_network_filterref_parameter_set_value(GVirSandboxConfigNetworkFilterrefParameter *param,
                                                               const gchar *value)
{
    g_return_if_fail(GVIR_SANDBOX_IS_CONFIG_NETWORK_FILTERREF_PARAMETER(param));

    g_free(param->priv->value);
    param->priv->value = g_strdup(value);
    g_object_notify(G_OBJECT(value), "value");
}

const gchar *gvir_sandbox_config_network_filterref_parameter_get_value(GVirSandboxConfigNetworkFilterrefParameter *param)
{
    g_return_val_if_fail(GVIR_SANDBOX_IS_CONFIG_NETWORK_FILTERREF_PARAMETER(param), NULL);

    return param->priv->value;
}
