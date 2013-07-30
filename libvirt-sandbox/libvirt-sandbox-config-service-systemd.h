/*
 * libvirt-sandbox-config-service-systemd.h: libvirt sandbox configuration
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

#ifndef __LIBVIRT_SANDBOX_CONFIG_SERVICE_SYSTEMD_H__
#define __LIBVIRT_SANDBOX_CONFIG_SERVICE_SYSTEMD_H__

G_BEGIN_DECLS

#define GVIR_SANDBOX_TYPE_CONFIG_SERVICE_SYSTEMD            (gvir_sandbox_config_service_systemd_get_type ())
#define GVIR_SANDBOX_CONFIG_SERVICE_SYSTEMD(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GVIR_SANDBOX_TYPE_CONFIG_SERVICE_SYSTEMD, GVirSandboxConfigServiceSystemd))
#define GVIR_SANDBOX_CONFIG_SERVICE_SYSTEMD_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GVIR_SANDBOX_TYPE_CONFIG_SERVICE_SYSTEMD, GVirSandboxConfigServiceSystemdClass))
#define GVIR_SANDBOX_IS_CONFIG_SERVICE_SYSTEMD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GVIR_SANDBOX_TYPE_CONFIG_SERVICE_SYSTEMD))
#define GVIR_SANDBOX_IS_CONFIG_SERVICE_SYSTEMD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GVIR_SANDBOX_TYPE_CONFIG_SERVICE_SYSTEMD))
#define GVIR_SANDBOX_CONFIG_SERVICE_SYSTEMD_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GVIR_SANDBOX_TYPE_CONFIG_SERVICE_SYSTEMD, GVirSandboxConfigServiceSystemdClass))

typedef struct _GVirSandboxConfigServiceSystemd GVirSandboxConfigServiceSystemd;
typedef struct _GVirSandboxConfigServiceSystemdPrivate GVirSandboxConfigServiceSystemdPrivate;
typedef struct _GVirSandboxConfigServiceSystemdClass GVirSandboxConfigServiceSystemdClass;

struct _GVirSandboxConfigServiceSystemd
{
    GVirSandboxConfigService parent;

    GVirSandboxConfigServiceSystemdPrivate *priv;

    /* Do not add fields to this struct */
};

struct _GVirSandboxConfigServiceSystemdClass
{
    GVirSandboxConfigServiceClass parent_class;

    gpointer padding[LIBVIRT_SANDBOX_CLASS_PADDING];
};

GType gvir_sandbox_config_service_systemd_get_type(void);

GVirSandboxConfigServiceSystemd *gvir_sandbox_config_service_systemd_new(const gchar *name);

const gchar *gvir_sandbox_config_service_systemd_get_boot_target(GVirSandboxConfigServiceSystemd *config);
void gvir_sandbox_config_service_systemd_set_boot_target(GVirSandboxConfigServiceSystemd *config,
                                                         const gchar *target);

G_END_DECLS

#endif /* __LIBVIRT_SANDBOX_CONFIG_SERVICE_SYSTEMD_H__ */
