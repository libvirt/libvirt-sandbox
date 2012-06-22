/*
 * libvirt-sandbox-config-mount-ram.c: libvirt sandbox configuration
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
 * SECTION: libvirt-sandbox-config-mount-ram
 * @short_description: Filesystem attachment configuration details
 * @include: libvirt-sandbox/libvirt-sandbox.h
 * @see_aloso: #GVirSandboxConfig
 *
 * Provides an object to store information about a filesystem attachment in the sandbox
 *
 * The GVirSandboxConfigMount object stores information required to attach
 * a host filesystem to the application sandbox. The sandbox starts off with
 * a complete view of the host filesystem. This object allows a specific
 * area of the host filesystem to be hidden and replaced with alternate
 * content.
 */

#define GVIR_SANDBOX_CONFIG_MOUNT_RAM_GET_PRIVATE(obj)                         \
        (G_TYPE_INSTANCE_GET_PRIVATE((obj), GVIR_SANDBOX_TYPE_CONFIG_MOUNT_RAM, GVirSandboxConfigMountRamPrivate))

struct _GVirSandboxConfigMountRamPrivate
{
    guint64 usage;
};

G_DEFINE_TYPE(GVirSandboxConfigMountRam, gvir_sandbox_config_mount_ram, GVIR_SANDBOX_TYPE_CONFIG_MOUNT);

enum {
    PROP_0,
    PROP_USAGE,
};

enum {
    LAST_SIGNAL
};

//static gint signals[LAST_SIGNAL];


static void gvir_sandbox_config_mount_ram_get_property(GObject *object,
                                                       guint prop_id,
                                                       GValue *value,
                                                       GParamSpec *pspec)
{
    GVirSandboxConfigMountRam *config = GVIR_SANDBOX_CONFIG_MOUNT_RAM(object);
    GVirSandboxConfigMountRamPrivate *priv = config->priv;

    switch (prop_id) {
    case PROP_USAGE:
        g_value_set_uint64(value, priv->usage);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_config_mount_ram_set_property(GObject *object,
                                                       guint prop_id,
                                                       const GValue *value,
                                                       GParamSpec *pspec)
{
    GVirSandboxConfigMountRam *config = GVIR_SANDBOX_CONFIG_MOUNT_RAM(object);
    GVirSandboxConfigMountRamPrivate *priv = config->priv;

    switch (prop_id) {
    case PROP_USAGE:
        priv->usage = g_value_get_uint64(value);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_config_mount_ram_class_init(GVirSandboxConfigMountRamClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->get_property = gvir_sandbox_config_mount_ram_get_property;
    object_class->set_property = gvir_sandbox_config_mount_ram_set_property;

    g_object_class_install_property(object_class,
                                    PROP_USAGE,
                                    g_param_spec_uint64("usage",
                                                        "Usage",
                                                        "The maximum ram usage (KiB)",
                                                        0,
                                                        G_MAXUINT64,
                                                        1024 * 10,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_type_class_add_private(klass, sizeof(GVirSandboxConfigMountRamPrivate));
}


static void gvir_sandbox_config_mount_ram_init(GVirSandboxConfigMountRam *config)
{
    GVirSandboxConfigMountRamPrivate *priv = config->priv;
    priv = config->priv = GVIR_SANDBOX_CONFIG_MOUNT_RAM_GET_PRIVATE(config);

    priv->usage = 1024 * 10;
}


/**
 * gvir_sandbox_config_mount_ram_new:
 * @targetdir: (transfer none): the target directory
 *
 * Create a new custom mount mapped to the directory @targetdir
 *
 * Returns: (transfer full): a new sandbox mount object
 */
GVirSandboxConfigMountRam *gvir_sandbox_config_mount_ram_new(const gchar *targetdir,
                                                             guint64 usage)
{
    return GVIR_SANDBOX_CONFIG_MOUNT_RAM(g_object_new(GVIR_SANDBOX_TYPE_CONFIG_MOUNT_RAM,
                                                      "target", targetdir,
                                                      "usage", usage,
                                                      NULL));
}


/**
 * gvir_sandbox_config_mount_ram_set_usage:
 * @config: (transfer none): the sandbox mount config
 * @usage: the memory usage limit in KiB
 *
 * Sets the memory usage limit for the RAM filesystem in Kibibytes
 */
void gvir_sandbox_config_mount_ram_set_usage(GVirSandboxConfigMountRam *config, guint64 usage)
{
    GVirSandboxConfigMountRamPrivate *priv = config->priv;
    priv->usage = usage;
}

/**
 * gvir_sandbox_config_mount_ram_get_usage:
 * @config: (transfer none): the sandbox mount config
 *
 * Retrieves the memory usage limit for the RAM filesystem in Kibibytes
 *
 * Returns: the usage limit
 */
guint64 gvir_sandbox_config_mount_ram_get_usage(GVirSandboxConfigMountRam *config)
{
    GVirSandboxConfigMountRamPrivate *priv = config->priv;
    return priv->usage;
}
