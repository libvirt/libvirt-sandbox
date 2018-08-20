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

#include "libvirt-sandbox/libvirt-sandbox-config-all.h"

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

#define GVIR_SANDBOX_CONFIG_MOUNT_HOST_IMAGE_GET_PRIVATE(obj)           \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), GVIR_SANDBOX_TYPE_CONFIG_MOUNT_HOST_IMAGE, GVirSandboxConfigMountHostImagePrivate))

struct _GVirSandboxConfigMountHostImagePrivate
{
    GVirConfigDomainDiskFormat format;
};

G_DEFINE_TYPE_WITH_PRIVATE(GVirSandboxConfigMountHostImage, gvir_sandbox_config_mount_host_image, GVIR_SANDBOX_TYPE_CONFIG_MOUNT_FILE);

enum {
    PROP_0,
    PROP_FORMAT,
};

enum {
    LAST_SIGNAL
};

//static gint signals[LAST_SIGNAL];


static void gvir_sandbox_config_mount_host_image_get_property(GObject *object,
                                                              guint prop_id,
                                                              GValue *value,
                                                              GParamSpec *pspec)
{
    GVirSandboxConfigMountHostImage *config = GVIR_SANDBOX_CONFIG_MOUNT_HOST_IMAGE(object);
    GVirSandboxConfigMountHostImagePrivate *priv = config->priv;

    switch (prop_id) {
    case PROP_FORMAT:
        g_value_set_enum(value, priv->format);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_config_mount_host_image_set_property(GObject *object,
                                                              guint prop_id,
                                                              const GValue *value,
                                                              GParamSpec *pspec)
{
    GVirSandboxConfigMountHostImage *config = GVIR_SANDBOX_CONFIG_MOUNT_HOST_IMAGE(object);
    GVirSandboxConfigMountHostImagePrivate *priv = config->priv;

    switch (prop_id) {
    case PROP_FORMAT:
        priv->format = g_value_get_enum(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_config_mount_host_image_class_init(GVirSandboxConfigMountHostImageClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->get_property = gvir_sandbox_config_mount_host_image_get_property;
    object_class->set_property = gvir_sandbox_config_mount_host_image_set_property;

    g_object_class_install_property(object_class,
                                    PROP_FORMAT,
                                    g_param_spec_enum("format",
                                                      "Disk format",
                                                      "The disk format",
                                                      GVIR_CONFIG_TYPE_DOMAIN_DISK_FORMAT,
                                                      GVIR_CONFIG_DOMAIN_DISK_FORMAT_RAW,
                                                      G_PARAM_READABLE |
                                                      G_PARAM_WRITABLE |
                                                      G_PARAM_CONSTRUCT_ONLY |
                                                      G_PARAM_STATIC_NAME |
                                                      G_PARAM_STATIC_NICK |
                                                      G_PARAM_STATIC_BLURB));
}


static void gvir_sandbox_config_mount_host_image_init(GVirSandboxConfigMountHostImage *config)
{
    GVirSandboxConfigMountHostImagePrivate *priv = config->priv;
    priv = config->priv = GVIR_SANDBOX_CONFIG_MOUNT_HOST_IMAGE_GET_PRIVATE(config);

    priv->format = GVIR_CONFIG_DOMAIN_DISK_FORMAT_RAW;
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
                                                                          const gchar *targetdir,
                                                                          GVirConfigDomainDiskFormat format)
{
    return GVIR_SANDBOX_CONFIG_MOUNT_HOST_IMAGE(g_object_new(GVIR_SANDBOX_TYPE_CONFIG_MOUNT_HOST_IMAGE,
                                                             "source", source,
                                                             "target", targetdir,
                                                             "format", format,
                                                             NULL));
}

/**
 * gvir_sandbox_config_mount_host_image_get_format:
 * @config: (transfer none): the sandbox mount config
 *
 * Retrieves the image format of the host-image filesystem.
 *
 * Returns: (transfer none): the imave format
 */
GVirConfigDomainDiskFormat gvir_sandbox_config_mount_host_image_get_format(GVirSandboxConfigMountHostImage *config)
{
    GVirSandboxConfigMountHostImagePrivate *priv = config->priv;
    return priv->format;
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
