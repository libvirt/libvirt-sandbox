/*
 * libvirt-sandbox-config-network-route.c: libvirt sandbox configuration
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 *
 * Author: Daniel P. Berrange <berrange@redhat.com>
 */

#include <config.h>
#include <string.h>
#include <sys/utsname.h>

#include "libvirt-sandbox/libvirt-sandbox.h"

/**
 * SECTION: libvirt-sandbox-config-network_route
 * @short_description: Route information for an interface
 * @include: libvirt-sandbox/libvirt-sandbox.h
 *
 * Provides an object to store information about a network interface route
 *
 * The GVirSandboxConfigNetworkRoute object stores the information required
 * to configure an route on a network interface. This compises the primary
 * route, broadcast route and network route.
 */

#define GVIR_SANDBOX_CONFIG_NETWORK_ROUTE_GET_PRIVATE(obj)                         \
        (G_TYPE_INSTANCE_GET_PRIVATE((obj), GVIR_SANDBOX_TYPE_CONFIG_NETWORK_ROUTE, GVirSandboxConfigNetworkRoutePrivate))

struct _GVirSandboxConfigNetworkRoutePrivate
{
    GInetAddress *netmask;
    gchar *gateway;
    GInetAddress *target;
};

G_DEFINE_TYPE(GVirSandboxConfigNetworkRoute, gvir_sandbox_config_network_route, G_TYPE_OBJECT);


enum {
    PROP_0,
    PROP_NETMASK,
    PROP_GATEWAY,
    PROP_TARGET,
};

enum {
    LAST_SIGNAL
};

//static gint signals[LAST_SIGNAL];


static void gvir_sandbox_config_network_route_get_property(GObject *object,
                                                           guint prop_id,
                                                           GValue *value,
                                                           GParamSpec *pspec)
{
    GVirSandboxConfigNetworkRoute *config = GVIR_SANDBOX_CONFIG_NETWORK_ROUTE(object);
    GVirSandboxConfigNetworkRoutePrivate *priv = config->priv;

    switch (prop_id) {
    case PROP_NETMASK:
        g_value_set_object(value, priv->netmask);
        break;

    case PROP_GATEWAY:
        g_value_set_string(value, priv->gateway);
        break;

    case PROP_TARGET:
        g_value_set_object(value, priv->target);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_config_network_route_set_property(GObject *object,
                                                             guint prop_id,
                                                             const GValue *value,
                                                             GParamSpec *pspec)
{
    GVirSandboxConfigNetworkRoute *config = GVIR_SANDBOX_CONFIG_NETWORK_ROUTE(object);
    GVirSandboxConfigNetworkRoutePrivate *priv = config->priv;

    switch (prop_id) {
    case PROP_NETMASK:
        if (priv->netmask)
            g_object_unref(priv->netmask);
        priv->netmask = g_value_dup_object(value);
        break;

    case PROP_GATEWAY:
        g_free(priv->gateway);
        priv->gateway = g_value_dup_string(value);
        break;

    case PROP_TARGET:
        if (priv->target)
            g_object_unref(priv->target);
        priv->target = g_value_dup_object(value);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_config_network_route_finalize(GObject *object)
{
    GVirSandboxConfigNetworkRoute *config = GVIR_SANDBOX_CONFIG_NETWORK_ROUTE(object);
    GVirSandboxConfigNetworkRoutePrivate *priv = config->priv;

    if (priv->netmask)
        g_object_unref(priv->netmask);
    g_free(priv->gateway);
    if (priv->target)
        g_object_unref(priv->target);

    G_OBJECT_CLASS(gvir_sandbox_config_network_route_parent_class)->finalize(object);
}


static void gvir_sandbox_config_network_route_class_init(GVirSandboxConfigNetworkRouteClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = gvir_sandbox_config_network_route_finalize;
    object_class->get_property = gvir_sandbox_config_network_route_get_property;
    object_class->set_property = gvir_sandbox_config_network_route_set_property;

    g_object_class_install_property(object_class,
                                    PROP_NETMASK,
                                    g_param_spec_object("netmask",
                                                        "Netmask",
                                                        "Netmask address",
                                                        G_TYPE_INET_ADDRESS,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_GATEWAY,
                                    g_param_spec_string("gateway",
                                                        "Gateway",
                                                        "Gateway device",
                                                        NULL,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_TARGET,
                                    g_param_spec_object("target",
                                                        "Target",
                                                        "Target address",
                                                        G_TYPE_INET_ADDRESS,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_type_class_add_private(klass, sizeof(GVirSandboxConfigNetworkRoutePrivate));
}


static void gvir_sandbox_config_network_route_init(GVirSandboxConfigNetworkRoute *config)
{
    config->priv = GVIR_SANDBOX_CONFIG_NETWORK_ROUTE_GET_PRIVATE(config);
}


/**
 * gvir_sandbox_config_network_route_new:
 * @netmask: the netmask route
 * @gateway: the gateway device
 * @target: the target address
 *
 * Create a new network route config.
 *
 * Returns: (transfer full): a new sandbox network route object
 */
GVirSandboxConfigNetworkRoute *gvir_sandbox_config_network_route_new(GInetAddress *netmask,
                                                                     const gchar *gateway,
                                                                     GInetAddress *target)
{
    return GVIR_SANDBOX_CONFIG_NETWORK_ROUTE(g_object_new(GVIR_SANDBOX_TYPE_CONFIG_NETWORK_ROUTE,
                                                          "netmask", netmask,
                                                          "gateway", gateway,
                                                          "target", target,
                                                          NULL));
}


/**
 * gvir_sandbox_config_network_route_set_netmask:
 * @config: (transfer none): the sandbox network route config
 * @addr: (transfer none): the netmask route
 *
 * Sets the netmask for an interface route
 */
void gvir_sandbox_config_network_route_set_netmask(GVirSandboxConfigNetworkRoute *config,
                                                   GInetAddress *addr)
{
    GVirSandboxConfigNetworkRoutePrivate *priv = config->priv;
    if (priv->netmask)
        g_object_unref(priv->netmask);
    priv->netmask = g_object_ref(addr);
}


/**
 * gvir_sandbox_config_network_route_get_netmask:
 * @config: (transfer none): the sandbox network route config
 *
 * Retrieves the device that is the netmask for the route
 *
 * Returns: (transfer none): the netmask address
 */
GInetAddress *gvir_sandbox_config_network_route_get_netmask(GVirSandboxConfigNetworkRoute *config)
{
    GVirSandboxConfigNetworkRoutePrivate *priv = config->priv;
    return priv->netmask;
}


/**
 * gvir_sandbox_config_network_route_set_gateway:
 * @config: (transfer none): the sandbox network route config
 * @dev: (transfer none): the gateway device
 *
 * Sets the interface gateway device
 */
void gvir_sandbox_config_network_route_set_gateway(GVirSandboxConfigNetworkRoute *config,
                                                   const gchar *dev)
{
    GVirSandboxConfigNetworkRoutePrivate *priv = config->priv;
    g_free(priv->gateway);
    priv->gateway = g_strdup(dev);
}


/**
 * gvir_sandbox_config_network_route_get_gateway:
 * @config: (transfer none): the sandbox network route config
 *
 * Retrieves the network gateway device
 *
 * Returns: (transfer none): the gateway device
 */
const gchar *gvir_sandbox_config_network_route_get_gateway(GVirSandboxConfigNetworkRoute *config)
{
    GVirSandboxConfigNetworkRoutePrivate *priv = config->priv;
    return priv->gateway;
}


/**
 * gvir_sandbox_config_network_route_set_target:
 * @config: (transfer none): the sandbox network route config
 * @addr: (transfer none): the target address
 *
 * Sets the interface route target address
 */
void gvir_sandbox_config_network_route_set_target(GVirSandboxConfigNetworkRoute *config,
                                                     GInetAddress *addr)
{
    GVirSandboxConfigNetworkRoutePrivate *priv = config->priv;
    if (priv->target)
        g_object_unref(priv->target);
    priv->target = g_object_ref(addr);
}


/**
 * gvir_sandbox_config_network_route_get_target:
 * @config: (transfer none): the sandbox network route config
 *
 * Retrieves the route target address
 *
 * Returns: (transfer none): the target address
 */
GInetAddress *gvir_sandbox_config_network_route_get_target(GVirSandboxConfigNetworkRoute *config)
{
    GVirSandboxConfigNetworkRoutePrivate *priv = config->priv;
    return priv->target;
}
