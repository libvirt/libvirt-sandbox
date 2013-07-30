/*
 * libvirt-sandbox-config-mount-host-image.h: libvirt sandbox configuration
 *
 * Copyright (C) 2011-2012 Red Hat, Inc.
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

#ifndef __LIBVIRT_SANDBOX_CONFIG_MOUNT_HOST_IMAGE_H__
#define __LIBVIRT_SANDBOX_CONFIG_MOUNT_HOST_IMAGE_H__

G_BEGIN_DECLS

#define GVIR_SANDBOX_TYPE_CONFIG_MOUNT_HOST_IMAGE            (gvir_sandbox_config_mount_host_image_get_type ())
#define GVIR_SANDBOX_CONFIG_MOUNT_HOST_IMAGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GVIR_SANDBOX_TYPE_CONFIG_MOUNT_HOST_IMAGE, GVirSandboxConfigMountHostImage))
#define GVIR_SANDBOX_CONFIG_MOUNT_HOST_IMAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GVIR_SANDBOX_TYPE_CONFIG_MOUNT_HOST_IMAGE, GVirSandboxConfigMountHostImageClass))
#define GVIR_SANDBOX_IS_CONFIG_MOUNT_HOST_IMAGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GVIR_SANDBOX_TYPE_CONFIG_MOUNT_HOST_IMAGE))
#define GVIR_SANDBOX_IS_CONFIG_MOUNT_HOST_IMAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GVIR_SANDBOX_TYPE_CONFIG_MOUNT_HOST_IMAGE))
#define GVIR_SANDBOX_CONFIG_MOUNT_HOST_IMAGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GVIR_SANDBOX_TYPE_CONFIG_MOUNT_HOST_IMAGE, GVirSandboxConfigMountHostImageClass))

typedef struct _GVirSandboxConfigMountHostImage GVirSandboxConfigMountHostImage;
typedef struct _GVirSandboxConfigMountHostImagePrivate GVirSandboxConfigMountHostImagePrivate;
typedef struct _GVirSandboxConfigMountHostImageClass GVirSandboxConfigMountHostImageClass;

struct _GVirSandboxConfigMountHostImage
{
    GVirSandboxConfigMountFile parent;

    GVirSandboxConfigMountHostImagePrivate *priv;

    /* Do not add fields to this struct */
};

struct _GVirSandboxConfigMountHostImageClass
{
    GVirSandboxConfigMountFileClass parent_class;

    gpointer padding[LIBVIRT_SANDBOX_CLASS_PADDING];
};

GType gvir_sandbox_config_mount_host_image_get_type(void);

GVirSandboxConfigMountHostImage *gvir_sandbox_config_mount_host_image_new(const gchar *source,
                                                                          const gchar *targetdir);

G_END_DECLS

#endif /* __LIBVIRT_SANDBOX_CONFIG_MOUNT_HOST_IMAGE_H__ */
