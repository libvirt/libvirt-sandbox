/*
 * libvirt-sandbox-config-service-generic.h: libvirt sandbox configuration
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

#ifndef __LIBVIRT_SANDBOX_CONFIG_SERVICE_GENERIC_H__
#define __LIBVIRT_SANDBOX_CONFIG_SERVICE_GENERIC_H__

G_BEGIN_DECLS

#define GVIR_SANDBOX_TYPE_CONFIG_SERVICE_GENERIC            (gvir_sandbox_config_service_generic_get_type ())
#define GVIR_SANDBOX_CONFIG_SERVICE_GENERIC(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GVIR_SANDBOX_TYPE_CONFIG_SERVICE_GENERIC, GVirSandboxConfigServiceGeneric))
#define GVIR_SANDBOX_CONFIG_SERVICE_GENERIC_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GVIR_SANDBOX_TYPE_CONFIG_SERVICE_GENERIC, GVirSandboxConfigServiceGenericClass))
#define GVIR_SANDBOX_IS_CONFIG_SERVICE_GENERIC(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GVIR_SANDBOX_TYPE_CONFIG_SERVICE_GENERIC))
#define GVIR_SANDBOX_IS_CONFIG_SERVICE_GENERIC_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GVIR_SANDBOX_TYPE_CONFIG_SERVICE_GENERIC))
#define GVIR_SANDBOX_CONFIG_SERVICE_GENERIC_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GVIR_SANDBOX_TYPE_CONFIG_SERVICE_GENERIC, GVirSandboxConfigServiceGenericClass))

typedef struct _GVirSandboxConfigServiceGeneric GVirSandboxConfigServiceGeneric;
typedef struct _GVirSandboxConfigServiceGenericPrivate GVirSandboxConfigServiceGenericPrivate;
typedef struct _GVirSandboxConfigServiceGenericClass GVirSandboxConfigServiceGenericClass;

struct _GVirSandboxConfigServiceGeneric
{
    GVirSandboxConfigService parent;

    GVirSandboxConfigServiceGenericPrivate *priv;

    /* Do not add fields to this struct */
};

struct _GVirSandboxConfigServiceGenericClass
{
    GVirSandboxConfigServiceClass parent_class;

    gpointer padding[LIBVIRT_SANDBOX_CLASS_PADDING];
};

GType gvir_sandbox_config_service_generic_get_type(void);

GVirSandboxConfigServiceGeneric *gvir_sandbox_config_service_generic_new(const gchar *name);

void gvir_sandbox_config_service_generic_set_command(GVirSandboxConfigServiceGeneric *config, gchar **argv);

G_END_DECLS

#endif /* __LIBVIRT_SANDBOX_CONFIG_SERVICE_GENERIC_H__ */
