/*
 * libvirt-sandbox-config-initrd.h: libvirt sandbox configuration
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

#ifndef __LIBVIRT_SANDBOX_CONFIG_INITRD_H__
#define __LIBVIRT_SANDBOX_CONFIG_INITRD_H__

G_BEGIN_DECLS

#define GVIR_SANDBOX_TYPE_CONFIG_INITRD            (gvir_sandbox_config_initrd_get_type ())
#define GVIR_SANDBOX_CONFIG_INITRD(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GVIR_SANDBOX_TYPE_CONFIG_INITRD, GVirSandboxConfigInitrd))
#define GVIR_SANDBOX_CONFIG_INITRD_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GVIR_SANDBOX_TYPE_CONFIG_INITRD, GVirSandboxConfigInitrdClass))
#define GVIR_SANDBOX_IS_CONFIG_INITRD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GVIR_SANDBOX_TYPE_CONFIG_INITRD))
#define GVIR_SANDBOX_IS_CONFIG_INITRD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GVIR_SANDBOX_TYPE_CONFIG_INITRD))
#define GVIR_SANDBOX_CONFIG_INITRD_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GVIR_SANDBOX_TYPE_CONFIG_INITRD, GVirSandboxConfigInitrdClass))

#define GVIR_SANDBOX_TYPE_CONFIG_INITRD_HANDLE      (gvir_sandbox_config_initrd_handle_get_type ())

typedef struct _GVirSandboxConfigInitrd GVirSandboxConfigInitrd;
typedef struct _GVirSandboxConfigInitrdPrivate GVirSandboxConfigInitrdPrivate;
typedef struct _GVirSandboxConfigInitrdClass GVirSandboxConfigInitrdClass;

struct _GVirSandboxConfigInitrd
{
    GObject parent;

    GVirSandboxConfigInitrdPrivate *priv;

    /* Do not add fields to this struct */
};

struct _GVirSandboxConfigInitrdClass
{
    GObjectClass parent_class;

    gpointer padding[LIBVIRT_SANDBOX_CLASS_PADDING];
};

GType gvir_sandbox_config_initrd_get_type(void);

GVirSandboxConfigInitrd *gvir_sandbox_config_initrd_new(void);

void gvir_sandbox_config_initrd_set_kver(GVirSandboxConfigInitrd *config, const gchar *version);
const gchar *gvir_sandbox_config_initrd_get_kver(GVirSandboxConfigInitrd *config);

void gvir_sandbox_config_initrd_set_kmoddir(GVirSandboxConfigInitrd *config, const gchar *kmoddir);
const gchar *gvir_sandbox_config_initrd_get_kmoddir(GVirSandboxConfigInitrd *config);

void gvir_sandbox_config_initrd_set_init(GVirSandboxConfigInitrd *config, const gchar *hostpath);
const gchar *gvir_sandbox_config_initrd_get_init(GVirSandboxConfigInitrd *config);

void gvir_sandbox_config_initrd_add_module(GVirSandboxConfigInitrd *config, const gchar *modname);
GList *gvir_sandbox_config_initrd_get_modules(GVirSandboxConfigInitrd *config);

G_END_DECLS

#endif /* __LIBVIRT_SANDBOX_CONFIG_INITRD_H__ */
