/*
 * libvirt-sandbox-config-network-filterref-parameter.h: libvirt sandbox configuration
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

#if !defined(__LIBVIRT_SANDBOX_H__) && !defined(LIBVIRT_SANDBOX_BUILD)
#error "Only <libvirt-sandbox/libvirt-sandbox.h> can be included directly."
#endif

#ifndef __LIBVIRT_SANDBOX_CONFIG_NETWORK_FILTERREF_PARAMETER_H__
#define __LIBVIRT_SANDBOX_CONFIG_NETWORK_FILTERREF_PARAMETER_H__

G_BEGIN_DECLS

#define GVIR_SANDBOX_TYPE_CONFIG_NETWORK_FILTERREF_PARAMETER            (gvir_sandbox_config_network_filterref_parameter_get_type ())
#define GVIR_SANDBOX_CONFIG_NETWORK_FILTERREF_PARAMETER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GVIR_SANDBOX_TYPE_CONFIG_NETWORK_FILTERREF_PARAMETER, GVirSandboxConfigNetworkFilterrefParameter))
#define GVIR_SANDBOX_CONFIG_NETWORK_FILTERREF_PARAMETER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GVIR_SANDBOX_TYPE_CONFIG_NETWORK_FILTERREF_PARAMETER, GVirSandboxConfigNetworkFilterrefParameterClass))
#define GVIR_SANDBOX_IS_CONFIG_NETWORK_FILTERREF_PARAMETER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GVIR_SANDBOX_TYPE_CONFIG_NETWORK_FILTERREF_PARAMETER))
#define GVIR_SANDBOX_IS_CONFIG_NETWORK_FILTERREF_PARAMETER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GVIR_SANDBOX_TYPE_CONFIG_NETWORK_FILTERREF_PARAMETER))
#define GVIR_SANDBOX_CONFIG_NETWORK_FILTERREF_PARAMETER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GVIR_SANDBOX_TYPE_CONFIG_NETWORK_FILTERREF_PARAMETER, GVirSandboxConfigNetworkFilterrefParameterClass))

typedef struct _GVirSandboxConfigNetworkFilterrefParameter GVirSandboxConfigNetworkFilterrefParameter;
typedef struct _GVirSandboxConfigNetworkFilterrefParameterPrivate GVirSandboxConfigNetworkFilterrefParameterPrivate;
typedef struct _GVirSandboxConfigNetworkFilterrefParameterClass GVirSandboxConfigNetworkFilterrefParameterClass;

struct _GVirSandboxConfigNetworkFilterrefParameter
{
    GObject parent;

    GVirSandboxConfigNetworkFilterrefParameterPrivate *priv;

    /* Do not add fields to this struct */
};

struct _GVirSandboxConfigNetworkFilterrefParameterClass
{
    GObjectClass parent_class;

    gpointer padding[LIBVIRT_SANDBOX_CLASS_PADDING];
};

GType gvir_sandbox_config_network_filterref_parameter_get_type(void);

GVirSandboxConfigNetworkFilterrefParameter *gvir_sandbox_config_network_filterref_parameter_new(void);

void gvir_sandbox_config_network_filterref_parameter_set_name(GVirSandboxConfigNetworkFilterrefParameter *param,
                                                              const gchar *name);
const gchar *gvir_sandbox_config_network_filterref_parameter_get_name(GVirSandboxConfigNetworkFilterrefParameter *param);

void gvir_sandbox_config_network_filterref_parameter_set_value(GVirSandboxConfigNetworkFilterrefParameter *param,
                                                               const gchar *value);
const gchar *gvir_sandbox_config_network_filterref_parameter_get_value(GVirSandboxConfigNetworkFilterrefParameter *param);

G_END_DECLS

#endif /* __LIBVIRT_SANDBOX_CONFIG_NETWORK_FILTERREF_PARAMETER_H__ */
