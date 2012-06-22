/*
 * libvirt-sandbox-config-mount-host-image.c: libvirt sandbox configuration
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

#include <config.h>
#include <string.h>

#include "libvirt-sandbox/libvirt-sandbox.h"

/**
 * SECTION: libvirt-sandbox-config-mount-image
 * @short_description: Imagesystem attachment configuration details
 * @include: libvirt-sandbox/libvirt-sandbox.h
 * @see_aloso: #GVirSandboxConfig
 *
 * Provides an object to store information about a imagesystem attachment in the sandbox
 *
 * The GVirSandboxConfigMount object stores information required to attach
 * a host imagesystem to the application sandbox. The sandbox starts off with
 * a complete view of the host imagesystem. This object allows a specific
 * area of the host imagesystem to be hidden and replaced with alternate
 * content.
 */

#define GVIR_SANDBOX_CONFIG_MOUNT_HOST_IMAGE_GET_PRIVATE(obj)                         \
        (G_TYPE_INSTANCE_GET_PRIVATE((obj), GVIR_SANDBOX_TYPE_CONFIG_MOUNT_HOST_IMAGE, GVirSandboxConfigMountHostImagePrivate))

struct _GVirSandboxConfigMountHostImagePrivate
{
    gboolean unused;
};

G_DEFINE_TYPE(GVirSandboxConfigMountHostImage, gvir_sandbox_config_mount_host_image, GVIR_SANDBOX_TYPE_CONFIG_MOUNT_FILE);


static void gvir_sandbox_config_mount_host_image_class_init(GVirSandboxConfigMountHostImageClass *klass)
{
    g_type_class_add_private(klass, sizeof(GVirSandboxConfigMountHostImagePrivate));
}


static void gvir_sandbox_config_mount_host_image_init(GVirSandboxConfigMountHostImage *config)
{
    config->priv = GVIR_SANDBOX_CONFIG_MOUNT_HOST_IMAGE_GET_PRIVATE(config);
}


/**
 * gvir_sandbox_config_mount_host_image_new:
 * @targetdir: (transfer none): the target directory
 *
 * Create a new custom mount mapped to the directory @targetdir
 *
 * Returns: (transfer full): a new sandbox mount object
 */
GVirSandboxConfigMountHostImage *gvir_sandbox_config_mount_host_image_new(const gchar *source,
                                                                          const gchar *targetdir)
{
    return GVIR_SANDBOX_CONFIG_MOUNT_HOST_IMAGE(g_object_new(GVIR_SANDBOX_TYPE_CONFIG_MOUNT_HOST_IMAGE,
                                                             "source", source,
                                                             "target", targetdir,
                                                             NULL));
}
