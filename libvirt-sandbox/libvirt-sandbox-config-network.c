/*
 * libvirt-sandbox-config-network.c: libvirt sandbox configuration
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

#include "libvirt-sandbox/libvirt-sandbox.h"

/**
 * SECTION: libvirt-sandbox-config-network
 * @short_description: Kernel ramdisk configuration details
 * @include: libvirt-sandbox/libvirt-sandbox.h
 *
 * Provides an object to store information about a kernel ramdisk
 *
 * The GVirSandboxConfigNetwork object stores the information required
 * to build a kernel ramdisk to use when booting a virtual machine
 * as a sandbox.
 */

#define GVIR_SANDBOX_CONFIG_NETWORK_GET_PRIVATE(obj)                         \
        (G_TYPE_INSTANCE_GET_PRIVATE((obj), GVIR_SANDBOX_TYPE_CONFIG_NETWORK, GVirSandboxConfigNetworkPrivate))

struct _GVirSandboxConfigNetworkPrivate
{
    gboolean dhcp;
    gchar *source;
    gchar *mac;
    GList *routes;
    GList *addrs;
};

G_DEFINE_TYPE(GVirSandboxConfigNetwork, gvir_sandbox_config_network, G_TYPE_OBJECT);


enum {
    PROP_0,
    PROP_DHCP,
    PROP_SOURCE,
    PROP_MAC,
};

enum {
    LAST_SIGNAL
};

//static gint signals[LAST_SIGNAL];


static void gvir_sandbox_config_network_get_property(GObject *object,
                                                     guint prop_id,
                                                     GValue *value,
                                                     GParamSpec *pspec)
{
    GVirSandboxConfigNetwork *config = GVIR_SANDBOX_CONFIG_NETWORK(object);
    GVirSandboxConfigNetworkPrivate *priv = config->priv;

    switch (prop_id) {
    case PROP_DHCP:
        g_value_set_boolean(value, priv->dhcp);
        break;

    case PROP_SOURCE:
        g_value_set_string(value, priv->source);
        break;

    case PROP_MAC:
        g_value_set_string(value, priv->mac);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_config_network_set_property(GObject *object,
                                                     guint prop_id,
                                                     const GValue *value,
                                                     GParamSpec *pspec)
{
    GVirSandboxConfigNetwork *config = GVIR_SANDBOX_CONFIG_NETWORK(object);
    GVirSandboxConfigNetworkPrivate *priv = config->priv;

    switch (prop_id) {
    case PROP_DHCP:
        priv->dhcp = g_value_get_boolean(value);
        break;

    case PROP_SOURCE:
        g_free(priv->source);
        priv->source = g_value_dup_string(value);
        break;

    case PROP_MAC:
        g_free(priv->mac);
        priv->mac = g_value_dup_string(value);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_config_network_finalize(GObject *object)
{
    GVirSandboxConfigNetwork *config = GVIR_SANDBOX_CONFIG_NETWORK(object);
    GVirSandboxConfigNetworkPrivate *priv = config->priv;

    g_free(priv->source);
    g_free(priv->mac);
    g_list_foreach(priv->addrs, (GFunc)g_object_unref, NULL);
    g_list_foreach(priv->routes, (GFunc)g_object_unref, NULL);

    G_OBJECT_CLASS(gvir_sandbox_config_network_parent_class)->finalize(object);
}


static void gvir_sandbox_config_network_class_init(GVirSandboxConfigNetworkClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = gvir_sandbox_config_network_finalize;
    object_class->get_property = gvir_sandbox_config_network_get_property;
    object_class->set_property = gvir_sandbox_config_network_set_property;

    g_object_class_install_property(object_class,
                                    PROP_DHCP,
                                    g_param_spec_boolean("dhcp",
                                                         "DHCP",
                                                         "Enable DHCP",
                                                         TRUE,
                                                         G_PARAM_READABLE |
                                                         G_PARAM_WRITABLE |
                                                         G_PARAM_STATIC_NAME |
                                                         G_PARAM_STATIC_NICK |
                                                         G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_SOURCE,
                                    g_param_spec_string("source",
                                                        "Source",
                                                        "Source network",
                                                        NULL,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_MAC,
                                    g_param_spec_string("mac",
                                                        "MAC",
                                                        "MAC address",
                                                        NULL,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_type_class_add_private(klass, sizeof(GVirSandboxConfigNetworkPrivate));
}


static void gvir_sandbox_config_network_init(GVirSandboxConfigNetwork *config)
{
    GVirSandboxConfigNetworkPrivate *priv;
    priv = config->priv = GVIR_SANDBOX_CONFIG_NETWORK_GET_PRIVATE(config);

    priv->dhcp = TRUE;
}


/**
 * gvir_sandbox_config_network_new:
 *
 * Create a new network config with DHCP enabled
 *
 * Returns: (transfer full): a new sandbox network object
 */
GVirSandboxConfigNetwork *gvir_sandbox_config_network_new(void)
{
    return GVIR_SANDBOX_CONFIG_NETWORK(g_object_new(GVIR_SANDBOX_TYPE_CONFIG_NETWORK,
                                                    NULL));
}


void gvir_sandbox_config_network_set_source(GVirSandboxConfigNetwork *config,
                                            const gchar *network)
{
    GVirSandboxConfigNetworkPrivate *priv = config->priv;
    g_free(priv->source);
    priv->source = g_strdup(network);
}


const gchar *gvir_sandbox_config_network_get_source(GVirSandboxConfigNetwork *config)
{
    GVirSandboxConfigNetworkPrivate *priv = config->priv;
    return priv->source;
}


void gvir_sandbox_config_network_set_mac(GVirSandboxConfigNetwork *config,
                                              const gchar *mac)
{
    GVirSandboxConfigNetworkPrivate *priv = config->priv;
    g_free(priv->mac);
    priv->mac = g_strdup(mac);
}


const gchar *gvir_sandbox_config_network_get_mac(GVirSandboxConfigNetwork *config)
{
    GVirSandboxConfigNetworkPrivate *priv = config->priv;
    return priv->mac;
}


void gvir_sandbox_config_network_set_dhcp(GVirSandboxConfigNetwork *config,
                                          gboolean dhcp)
{
    GVirSandboxConfigNetworkPrivate *priv = config->priv;
    priv->dhcp = dhcp;
}


gboolean gvir_sandbox_config_network_get_dhcp(GVirSandboxConfigNetwork *config)
{
    GVirSandboxConfigNetworkPrivate *priv = config->priv;
    return priv->dhcp;
}


/**
 * gvir_sandbox_config_network_add_address:
 * @config: (transfer none): the sandbox network configuration
 * @addr: (transfer none): the network address
 *
 * Add a network interface address. This will be ignored unless
 * DHCP has been disabled
 */
void gvir_sandbox_config_network_add_address(GVirSandboxConfigNetwork *config,
                                             GVirSandboxConfigNetworkAddress *addr)
{
    GVirSandboxConfigNetworkPrivate *priv = config->priv;
    priv->addrs = g_list_append(priv->addrs, g_object_ref(addr));
}


/**
 * gvir_sandbox_config_network_get_addresses:
 * @config: (transfer none): the sandbox network configuration
 *
 * Retrieve the list of network interface addresses
 *
 * Returns: (transfer full)(element-type GVirSandboxConfigNetworkAddress): the address list
 */
GList *gvir_sandbox_config_network_get_addresses(GVirSandboxConfigNetwork *config)
{
    GVirSandboxConfigNetworkPrivate *priv = config->priv;
    g_list_foreach(priv->addrs, (GFunc)g_object_ref, NULL);
    return g_list_copy(priv->addrs);
}

/**
 * gvir_sandbox_config_network_add_route:
 * @config: (transfer none): the sandbox network configuration
 * @addr: (transfer none): the network route
 *
 * Add a network interface route. This will be ignored unless
 * DHCP has been disabled
 */
void gvir_sandbox_config_network_add_route(GVirSandboxConfigNetwork *config,
                                           GVirSandboxConfigNetworkRoute *route)
{
    GVirSandboxConfigNetworkPrivate *priv = config->priv;
    priv->routes = g_list_append(priv->routes, g_object_ref(route));
}

/**
 * gvir_sandbox_config_network_get_routes:
 * @config: (transfer none): the sandbox network configuration
 *
 * Retrieve the list of network interface routes
 *
 * Returns: (transfer full)(element-type GVirSandboxConfigNetworkRoute): the route list
 */
GList *gvir_sandbox_config_network_get_routes(GVirSandboxConfigNetwork *config)
{
    GVirSandboxConfigNetworkPrivate *priv = config->priv;
    g_list_foreach(priv->routes, (GFunc)g_object_ref, NULL);
    return g_list_copy(priv->routes);
}
