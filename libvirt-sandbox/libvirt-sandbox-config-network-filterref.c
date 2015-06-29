/*
 * libvirt-sandbox-config-network-filterref.c: libvirt sandbox filterr reference
 * configuration
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
 * SECTION: libvirt-sandbox-config-network-filterref
 * @short_description: Add a network filter to a network interface.
 * @include: libvirt-sandbox/libvirt-sandbox.h
 * @see_aloso: #GVirSandboxConfig
 *
 * Provides an object to store the name of the filter reference.
 *
 * The GVirSandboxConfigNetworkFilterref object stores the name of the filter
 * references associated with a network interface.
 */

#define GVIR_SANDBOX_CONFIG_NETWORK_FILTERREF_GET_PRIVATE(obj)                         \
        (G_TYPE_INSTANCE_GET_PRIVATE((obj), GVIR_SANDBOX_TYPE_CONFIG_NETWORK_FILTERREF, GVirSandboxConfigNetworkFilterrefPrivate))

struct _GVirSandboxConfigNetworkFilterrefPrivate
{
    gchar *filter;
    GList *parameters;
};

G_DEFINE_TYPE(GVirSandboxConfigNetworkFilterref, gvir_sandbox_config_network_filterref, G_TYPE_OBJECT);


enum {
    PROP_0,
    PROP_NAME
};

enum {
    LAST_SIGNAL
};

//static gint signals[LAST_SIGNAL];

static void gvir_sandbox_config_network_filterref_get_property(GObject *object,
                                                               guint prop_id,
                                                               GValue *value,
                                                               GParamSpec *pspec)
{
    GVirSandboxConfigNetworkFilterref *config = GVIR_SANDBOX_CONFIG_NETWORK_FILTERREF(object);
    GVirSandboxConfigNetworkFilterrefPrivate *priv = config->priv;

    switch (prop_id) {
    case PROP_NAME:
        g_value_set_string(value, priv->filter);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_config_network_filterref_set_property(GObject *object,
                                                               guint prop_id,
                                                               const GValue *value,
                                                               GParamSpec *pspec)
{
    GVirSandboxConfigNetworkFilterref *filterref = GVIR_SANDBOX_CONFIG_NETWORK_FILTERREF(object);

    switch (prop_id) {
    case PROP_NAME:
        gvir_sandbox_config_network_filterref_set_name(filterref, g_value_get_string(value));
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}



static void gvir_sandbox_config_network_filterref_finalize(GObject *object)
{
    GVirSandboxConfigNetworkFilterref *config = GVIR_SANDBOX_CONFIG_NETWORK_FILTERREF(object);
    GVirSandboxConfigNetworkFilterrefPrivate *priv = config->priv;

    g_free(priv->filter);
    g_list_foreach(priv->parameters, (GFunc)g_object_unref, NULL);
    g_list_free(priv->parameters);

    G_OBJECT_CLASS(gvir_sandbox_config_network_filterref_parent_class)->finalize(object);
}


static void gvir_sandbox_config_network_filterref_class_init(GVirSandboxConfigNetworkFilterrefClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = gvir_sandbox_config_network_filterref_finalize;
    object_class->get_property = gvir_sandbox_config_network_filterref_get_property;
    object_class->set_property = gvir_sandbox_config_network_filterref_set_property;

    g_object_class_install_property(object_class,
                                    PROP_NAME,
                                    g_param_spec_string("name",
                                                        "Filter name",
                                                        "The filter reference name",
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_STRINGS));

    g_type_class_add_private(klass, sizeof(GVirSandboxConfigNetworkFilterrefPrivate));
}

/**
 * gvir_sandbox_config_network_filterref_new:
 *
 * Create a new network filterref config.
 *
 * Returns: (transfer full): a new sandbox network_filterref object
 */
GVirSandboxConfigNetworkFilterref *gvir_sandbox_config_network_filterref_new(void)
{
    return GVIR_SANDBOX_CONFIG_NETWORK_FILTERREF(g_object_new(GVIR_SANDBOX_TYPE_CONFIG_NETWORK_FILTERREF,
                                                              NULL));
}


static void gvir_sandbox_config_network_filterref_init(GVirSandboxConfigNetworkFilterref *config)
{
    config->priv = GVIR_SANDBOX_CONFIG_NETWORK_FILTERREF_GET_PRIVATE(config);
}


/**
 * gvir_sandbox_config_network_filterref_get_name:
 * @config: (transfer none): the network filter reference name
 *
 * Retrieves the network filter reference name.
 *
 * Returns: (transfer none): the network filter reference name.
 */
const gchar *gvir_sandbox_config_network_filterref_get_name(GVirSandboxConfigNetworkFilterref *filterref)
{
    g_return_val_if_fail(GVIR_SANDBOX_IS_CONFIG_NETWORK_FILTERREF(filterref), NULL);

    return filterref->priv->filter;
}

void gvir_sandbox_config_network_filterref_set_name(GVirSandboxConfigNetworkFilterref *filterref,
                                                    const gchar *name)
{
    g_return_if_fail(GVIR_SANDBOX_IS_CONFIG_NETWORK_FILTERREF(filterref));

    g_free(filterref->priv->filter);
    filterref->priv->filter = g_strdup(name);
    g_object_notify(G_OBJECT(filterref), "name");
}

/**
 * gvir_sandbox_config_network_filterref_add_parameter:
 * @filter: (transfer none): the network filter reference.
 * @param: (transfer none): the filter parameter
 *
 * Add a parameter to a network filter reference.
 */
void gvir_sandbox_config_network_filterref_add_parameter(GVirSandboxConfigNetworkFilterref *filter,
                                                         GVirSandboxConfigNetworkFilterrefParameter *param)
{
    GVirSandboxConfigNetworkFilterrefPrivate *priv;

    g_return_if_fail(GVIR_SANDBOX_IS_CONFIG_NETWORK_FILTERREF(filter));
    g_return_if_fail(GVIR_SANDBOX_IS_CONFIG_NETWORK_FILTERREF_PARAMETER(param));
    priv = filter->priv;
    priv->parameters = g_list_append(priv->parameters, g_object_ref(param));
}


/**
 * gvir_sandbox_config_network_filterref_get_parameters:
 * @filter: (transfer none): the filter reference configuration
 *
 * Retrieve the list of parameters associated with a network filter reference
 *
 * Returns: (transfer full)(element-type GVirSandboxConfigNetworkFilterrefParameter): the parameter list
 */
GList *gvir_sandbox_config_network_filterref_get_parameters(GVirSandboxConfigNetworkFilterref *filter)
{
    GVirSandboxConfigNetworkFilterrefPrivate *priv;

    g_return_val_if_fail(GVIR_SANDBOX_IS_CONFIG_NETWORK_FILTERREF(filter), NULL);
    priv = filter->priv;
    g_list_foreach(priv->parameters, (GFunc)g_object_ref, NULL);
    return g_list_copy(priv->parameters);
}
