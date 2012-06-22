/*
 * libvirt-sandbox-config-mount-file.c: libvirt sandbox configuration
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
 * SECTION: libvirt-sandbox-config-mount-file
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

#define GVIR_SANDBOX_CONFIG_MOUNT_FILE_GET_PRIVATE(obj)                         \
        (G_TYPE_INSTANCE_GET_PRIVATE((obj), GVIR_SANDBOX_TYPE_CONFIG_MOUNT_FILE, GVirSandboxConfigMountFilePrivate))

struct _GVirSandboxConfigMountFilePrivate
{
    gchar *source;
};

G_DEFINE_ABSTRACT_TYPE(GVirSandboxConfigMountFile, gvir_sandbox_config_mount_file, GVIR_SANDBOX_TYPE_CONFIG_MOUNT);

enum {
    PROP_0,
    PROP_SOURCE,
};

enum {
    LAST_SIGNAL
};

//static gint signals[LAST_SIGNAL];


static void gvir_sandbox_config_mount_file_get_property(GObject *object,
                                                        guint prop_id,
                                                        GValue *value,
                                                        GParamSpec *pspec)
{
    GVirSandboxConfigMountFile *config = GVIR_SANDBOX_CONFIG_MOUNT_FILE(object);
    GVirSandboxConfigMountFilePrivate *priv = config->priv;

    switch (prop_id) {
    case PROP_SOURCE:
        g_value_set_string(value, priv->source);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_config_mount_file_set_property(GObject *object,
                                                        guint prop_id,
                                                        const GValue *value,
                                                        GParamSpec *pspec)
{
    GVirSandboxConfigMountFile *config = GVIR_SANDBOX_CONFIG_MOUNT_FILE(object);
    GVirSandboxConfigMountFilePrivate *priv = config->priv;

    switch (prop_id) {
    case PROP_SOURCE:
        g_free(priv->source);
        priv->source = g_value_dup_string(value);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_config_mount_file_finalize(GObject *object)
{
    GVirSandboxConfigMountFile *config = GVIR_SANDBOX_CONFIG_MOUNT_FILE(object);
    GVirSandboxConfigMountFilePrivate *priv = config->priv;

    g_free(priv->source);

    G_OBJECT_CLASS(gvir_sandbox_config_mount_file_parent_class)->finalize(object);
}


static void gvir_sandbox_config_mount_file_class_init(GVirSandboxConfigMountFileClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = gvir_sandbox_config_mount_file_finalize;
    object_class->get_property = gvir_sandbox_config_mount_file_get_property;
    object_class->set_property = gvir_sandbox_config_mount_file_set_property;

    g_object_class_install_property(object_class,
                                    PROP_SOURCE,
                                    g_param_spec_string("source",
                                                        "Source",
                                                        "The source directory",
                                                        NULL,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_type_class_add_private(klass, sizeof(GVirSandboxConfigMountFilePrivate));
}


static void gvir_sandbox_config_mount_file_init(GVirSandboxConfigMountFile *config)
{
    config->priv = GVIR_SANDBOX_CONFIG_MOUNT_FILE_GET_PRIVATE(config);
}


/**
 * gvir_sandbox_config_mount_file_set_source:
 * @config: (transfer none): the sandbox mount config
 * @source: (transfer none): the host directory
 *
 * Sets the directory to map to the custom mount. If no
 * directory is specified, an empty temporary directory will
 * be created
 */
void gvir_sandbox_config_mount_file_set_source(GVirSandboxConfigMountFile *config, const gchar *source)
{
    GVirSandboxConfigMountFilePrivate *priv = config->priv;
    g_free(priv->source);
    priv->source = g_strdup(source);
}

/**
 * gvir_sandbox_config_mount_file_get_source:
 * @config: (transfer none): the sandbox mount config
 *
 * Retrieves the directory mapped to the mount (if any)
 *
 * Returns: (transfer none): the source directory
 */
const gchar *gvir_sandbox_config_mount_file_get_source(GVirSandboxConfigMountFile *config)
{
    GVirSandboxConfigMountFilePrivate *priv = config->priv;
    return priv->source;
}
