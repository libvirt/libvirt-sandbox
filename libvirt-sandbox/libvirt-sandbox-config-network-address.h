/*
 * libvirt-sandbox-config-network-address.h: libvirt sandbox configuration
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

#ifndef __LIBVIRT_SANDBOX_CONFIG_NETWORK_ADDRESS_H__
#define __LIBVIRT_SANDBOX_CONFIG_NETWORK_ADDRESS_H__

G_BEGIN_DECLS

#define GVIR_SANDBOX_TYPE_CONFIG_NETWORK_ADDRESS            (gvir_sandbox_config_network_address_get_type ())
#define GVIR_SANDBOX_CONFIG_NETWORK_ADDRESS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GVIR_SANDBOX_TYPE_CONFIG_NETWORK_ADDRESS, GVirSandboxConfigNetworkAddress))
#define GVIR_SANDBOX_CONFIG_NETWORK_ADDRESS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GVIR_SANDBOX_TYPE_CONFIG_NETWORK_ADDRESS, GVirSandboxConfigNetworkAddressClass))
#define GVIR_SANDBOX_IS_CONFIG_NETWORK_ADDRESS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GVIR_SANDBOX_TYPE_CONFIG_NETWORK_ADDRESS))
#define GVIR_SANDBOX_IS_CONFIG_NETWORK_ADDRESS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GVIR_SANDBOX_TYPE_CONFIG_NETWORK_ADDRESS))
#define GVIR_SANDBOX_CONFIG_NETWORK_ADDRESS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GVIR_SANDBOX_TYPE_CONFIG_NETWORK_ADDRESS, GVirSandboxConfigNetworkAddressClass))

#define GVIR_SANDBOX_TYPE_CONFIG_NETWORK_ADDRESS_HANDLE      (gvir_sandbox_config_network_address_handle_get_type ())

typedef struct _GVirSandboxConfigNetworkAddress GVirSandboxConfigNetworkAddress;
typedef struct _GVirSandboxConfigNetworkAddressPrivate GVirSandboxConfigNetworkAddressPrivate;
typedef struct _GVirSandboxConfigNetworkAddressClass GVirSandboxConfigNetworkAddressClass;

struct _GVirSandboxConfigNetworkAddress
{
    GObject parent;

    GVirSandboxConfigNetworkAddressPrivate *priv;

    /* Do not add fields to this struct */
};

struct _GVirSandboxConfigNetworkAddressClass
{
    GObjectClass parent_class;

    gpointer padding[LIBVIRT_SANDBOX_CLASS_PADDING];
};

GType gvir_sandbox_config_network_address_get_type(void);

GVirSandboxConfigNetworkAddress *gvir_sandbox_config_network_address_new(GInetAddress *primary,
                                                                         guint prefix,
                                                                         GInetAddress *broadcast);

void gvir_sandbox_config_network_address_set_primary(GVirSandboxConfigNetworkAddress *config, GInetAddress *addr);
GInetAddress *gvir_sandbox_config_network_address_get_primary(GVirSandboxConfigNetworkAddress *config);

void gvir_sandbox_config_network_address_set_prefix(GVirSandboxConfigNetworkAddress *config, guint prefix);
guint gvir_sandbox_config_network_address_get_prefix(GVirSandboxConfigNetworkAddress *config);

void gvir_sandbox_config_network_address_set_broadcast(GVirSandboxConfigNetworkAddress *config, GInetAddress *addr);
GInetAddress *gvir_sandbox_config_network_address_get_broadcast(GVirSandboxConfigNetworkAddress *config);

G_END_DECLS

#endif /* __LIBVIRT_SANDBOX_CONFIG_NETWORK_ADDRESS_H__ */
