/*
 * libvirt-sandbox-config-network.h: libvirt sandbox configuration
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

#ifndef __LIBVIRT_SANDBOX_CONFIG_NETWORK_H__
#define __LIBVIRT_SANDBOX_CONFIG_NETWORK_H__

G_BEGIN_DECLS

#define GVIR_SANDBOX_TYPE_CONFIG_NETWORK            (gvir_sandbox_config_network_get_type ())
#define GVIR_SANDBOX_CONFIG_NETWORK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GVIR_SANDBOX_TYPE_CONFIG_NETWORK, GVirSandboxConfigNetwork))
#define GVIR_SANDBOX_CONFIG_NETWORK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GVIR_SANDBOX_TYPE_CONFIG_NETWORK, GVirSandboxConfigNetworkClass))
#define GVIR_SANDBOX_IS_CONFIG_NETWORK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GVIR_SANDBOX_TYPE_CONFIG_NETWORK))
#define GVIR_SANDBOX_IS_CONFIG_NETWORK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GVIR_SANDBOX_TYPE_CONFIG_NETWORK))
#define GVIR_SANDBOX_CONFIG_NETWORK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GVIR_SANDBOX_TYPE_CONFIG_NETWORK, GVirSandboxConfigNetworkClass))

#define GVIR_SANDBOX_TYPE_CONFIG_NETWORK_HANDLE      (gvir_sandbox_config_network_handle_get_type ())

typedef struct _GVirSandboxConfigNetwork GVirSandboxConfigNetwork;
typedef struct _GVirSandboxConfigNetworkPrivate GVirSandboxConfigNetworkPrivate;
typedef struct _GVirSandboxConfigNetworkClass GVirSandboxConfigNetworkClass;

struct _GVirSandboxConfigNetwork
{
    GObject parent;

    GVirSandboxConfigNetworkPrivate *priv;

    /* Do not add fields to this struct */
};

struct _GVirSandboxConfigNetworkClass
{
    GObjectClass parent_class;

    gpointer padding[LIBVIRT_SANDBOX_CLASS_PADDING];
};

GType gvir_sandbox_config_network_get_type(void);

GVirSandboxConfigNetwork *gvir_sandbox_config_network_new(void);

void gvir_sandbox_config_network_set_source(GVirSandboxConfigNetwork *config,
                                             const gchar *network);
const gchar *gvir_sandbox_config_network_get_source(GVirSandboxConfigNetwork *config);

void gvir_sandbox_config_network_set_mac(GVirSandboxConfigNetwork *config,
                                         const gchar *mac);
const gchar *gvir_sandbox_config_network_get_mac(GVirSandboxConfigNetwork *config);

void gvir_sandbox_config_network_set_dhcp(GVirSandboxConfigNetwork *config,
                                          gboolean dhcp);
gboolean gvir_sandbox_config_network_get_dhcp(GVirSandboxConfigNetwork *config);

void gvir_sandbox_config_network_add_address(GVirSandboxConfigNetwork *config,
                                             GVirSandboxConfigNetworkAddress *addr);
GList *gvir_sandbox_config_network_get_addresses(GVirSandboxConfigNetwork *config);

void gvir_sandbox_config_network_add_route(GVirSandboxConfigNetwork *config,
                                           GVirSandboxConfigNetworkRoute *addr);
GList *gvir_sandbox_config_network_get_routes(GVirSandboxConfigNetwork *config);


G_END_DECLS

#endif /* __LIBVIRT_SANDBOX_CONFIG_NETWORK_H__ */
