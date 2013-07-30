/*
 * libvirt-sandbox-config-network-route.h: libvirt sandbox configuration
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

#if !defined(__LIBVIRT_SANDBOX_H__) && !defined(LIBVIRT_SANDBOX_BUILD)
#error "Only <libvirt-sandbox/libvirt-sandbox.h> can be included directly."
#endif

#ifndef __LIBVIRT_SANDBOX_CONFIG_NETWORK_ROUTE_H__
#define __LIBVIRT_SANDBOX_CONFIG_NETWORK_ROUTE_H__

G_BEGIN_DECLS

#define GVIR_SANDBOX_TYPE_CONFIG_NETWORK_ROUTE            (gvir_sandbox_config_network_route_get_type ())
#define GVIR_SANDBOX_CONFIG_NETWORK_ROUTE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GVIR_SANDBOX_TYPE_CONFIG_NETWORK_ROUTE, GVirSandboxConfigNetworkRoute))
#define GVIR_SANDBOX_CONFIG_NETWORK_ROUTE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GVIR_SANDBOX_TYPE_CONFIG_NETWORK_ROUTE, GVirSandboxConfigNetworkRouteClass))
#define GVIR_SANDBOX_IS_CONFIG_NETWORK_ROUTE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GVIR_SANDBOX_TYPE_CONFIG_NETWORK_ROUTE))
#define GVIR_SANDBOX_IS_CONFIG_NETWORK_ROUTE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GVIR_SANDBOX_TYPE_CONFIG_NETWORK_ROUTE))
#define GVIR_SANDBOX_CONFIG_NETWORK_ROUTE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GVIR_SANDBOX_TYPE_CONFIG_NETWORK_ROUTE, GVirSandboxConfigNetworkRouteClass))

#define GVIR_SANDBOX_TYPE_CONFIG_NETWORK_ROUTE_HANDLE      (gvir_sandbox_config_network_route_handle_get_type ())

typedef struct _GVirSandboxConfigNetworkRoute GVirSandboxConfigNetworkRoute;
typedef struct _GVirSandboxConfigNetworkRoutePrivate GVirSandboxConfigNetworkRoutePrivate;
typedef struct _GVirSandboxConfigNetworkRouteClass GVirSandboxConfigNetworkRouteClass;

struct _GVirSandboxConfigNetworkRoute
{
    GObject parent;

    GVirSandboxConfigNetworkRoutePrivate *priv;

    /* Do not add fields to this struct */
};

struct _GVirSandboxConfigNetworkRouteClass
{
    GObjectClass parent_class;

    gpointer padding[LIBVIRT_SANDBOX_CLASS_PADDING];
};

GType gvir_sandbox_config_network_route_get_type(void);

GVirSandboxConfigNetworkRoute *gvir_sandbox_config_network_route_new(GInetAddress *target,
                                                                     guint prefix,
                                                                     GInetAddress *gateway);

void gvir_sandbox_config_network_route_set_prefix(GVirSandboxConfigNetworkRoute *config, guint prefix);
guint gvir_sandbox_config_network_route_get_prefix(GVirSandboxConfigNetworkRoute *config);

void gvir_sandbox_config_network_route_set_gateway(GVirSandboxConfigNetworkRoute *config, GInetAddress *addr);
GInetAddress *gvir_sandbox_config_network_route_get_gateway(GVirSandboxConfigNetworkRoute *config);

void gvir_sandbox_config_network_route_set_target(GVirSandboxConfigNetworkRoute *config, GInetAddress *addr);
GInetAddress *gvir_sandbox_config_network_route_get_target(GVirSandboxConfigNetworkRoute *config);

G_END_DECLS

#endif /* __LIBVIRT_SANDBOX_CONFIG_NETWORK_ROUTE_H__ */
