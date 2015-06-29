/*
 * libvirt-sandbox-config-disk.c: libvirt sandbox configuration
 *
 * Copyright (C) 2015 Universitat Politècnica de Catalunya.
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
 * Author: Eren Yagdiran <erenyagdiran@gmail.com>
 */

#include <config.h>
#include <string.h>

#include "libvirt-sandbox/libvirt-sandbox-config-all.h"

/**
 * SECTION: libvirt-sandbox-config-disk
 * @short_description: Disk attachment configuration details
 * @include: libvirt-sandbox/libvirt-sandbox.h
 * @see_aloso: #GVirSandboxConfig
 *
 * Provides an object to store information about a disk attachment in the sandbox
 *
 */

#define GVIR_SANDBOX_CONFIG_DISK_GET_PRIVATE(obj)                      \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), GVIR_SANDBOX_TYPE_CONFIG_DISK, GVirSandboxConfigDiskPrivate))


struct _GVirSandboxConfigDiskPrivate
{
    GVirConfigDomainDiskType type;
    gchar *tag;
    gchar *source;
    GVirConfigDomainDiskFormat format;
};

G_DEFINE_TYPE(GVirSandboxConfigDisk, gvir_sandbox_config_disk, G_TYPE_OBJECT);


enum {
    PROP_0,
    PROP_TYPE,
    PROP_TAG,
    PROP_SOURCE,
    PROP_FORMAT
};

enum {
    LAST_SIGNAL
};



static void gvir_sandbox_config_disk_get_property(GObject *object,
                                                  guint prop_id,
                                                  GValue *value,
                                                  GParamSpec *pspec)
{
    GVirSandboxConfigDisk *config = GVIR_SANDBOX_CONFIG_DISK(object);
    GVirSandboxConfigDiskPrivate *priv = config->priv;

    switch (prop_id) {
    case PROP_TAG:
        g_value_set_string(value, priv->tag);
        break;
    case PROP_SOURCE:
        g_value_set_string(value, priv->source);
        break;
    case PROP_TYPE:
        g_value_set_enum(value, priv->type);
        break;
    case PROP_FORMAT:
        g_value_set_enum(value, priv->format);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_config_disk_set_property(GObject *object,
                                                  guint prop_id,
                                                  const GValue *value,
                                                  GParamSpec *pspec)
{
    GVirSandboxConfigDisk *config = GVIR_SANDBOX_CONFIG_DISK(object);
    GVirSandboxConfigDiskPrivate *priv = config->priv;

    switch (prop_id) {
    case PROP_TAG:
        g_free(priv->tag);
        priv->tag = g_value_dup_string(value);
        break;
    case PROP_SOURCE:
        g_free(priv->source);
        priv->source = g_value_dup_string(value);
        break;
    case PROP_TYPE:
        priv->type = g_value_get_enum(value);
        break;
    case PROP_FORMAT:
        priv->format = g_value_get_enum(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_config_disk_finalize(GObject *object)
{
    GVirSandboxConfigDisk *config = GVIR_SANDBOX_CONFIG_DISK(object);
    GVirSandboxConfigDiskPrivate *priv = config->priv;

    g_free(priv->tag);
    g_free(priv->source);

    G_OBJECT_CLASS(gvir_sandbox_config_disk_parent_class)->finalize(object);
}


static void gvir_sandbox_config_disk_class_init(GVirSandboxConfigDiskClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = gvir_sandbox_config_disk_finalize;
    object_class->get_property = gvir_sandbox_config_disk_get_property;
    object_class->set_property = gvir_sandbox_config_disk_set_property;

    g_object_class_install_property(object_class,
                                    PROP_TAG,
                                    g_param_spec_string("tag",
                                                        "Tag",
                                                        "The sandbox tag property",
                                                        NULL,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_SOURCE,
                                    g_param_spec_string("source",
                                                        "Source",
                                                        "The sandbox source directory",
                                                        NULL,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));


    g_object_class_install_property(object_class,
                                    PROP_TYPE,
                                    g_param_spec_enum("type",
                                                      "Disk type",
                                                      "The sandbox disk type",
                                                      GVIR_CONFIG_TYPE_DOMAIN_DISK_TYPE,
                                                      GVIR_CONFIG_DOMAIN_DISK_FILE,
                                                      G_PARAM_READABLE |
                                                      G_PARAM_WRITABLE |
                                                      G_PARAM_CONSTRUCT_ONLY |
                                                      G_PARAM_STATIC_NAME |
                                                      G_PARAM_STATIC_NICK |
                                                      G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_FORMAT,
                                    g_param_spec_enum("format",
                                                      "Disk format",
                                                      "The sandbox disk format",
                                                      GVIR_CONFIG_TYPE_DOMAIN_DISK_FORMAT,
                                                      GVIR_CONFIG_DOMAIN_DISK_FORMAT_RAW,
                                                      G_PARAM_READABLE |
                                                      G_PARAM_WRITABLE |
                                                      G_PARAM_CONSTRUCT_ONLY |
                                                      G_PARAM_STATIC_NAME |
                                                      G_PARAM_STATIC_NICK |
                                                      G_PARAM_STATIC_BLURB));

    g_type_class_add_private(klass, sizeof(GVirSandboxConfigDiskPrivate));
}


static void gvir_sandbox_config_disk_init(GVirSandboxConfigDisk *config)
{
    config->priv = GVIR_SANDBOX_CONFIG_DISK_GET_PRIVATE(config);
}

/**
 * gvir_sandbox_config_disk_get_disk_type:
 * @config: (transfer none): the sandbox disk config
 *
 * Retrieves the type property for the custom disk
 *
 * Returns: (transfer none): the type property of disk
 */
GVirConfigDomainDiskType gvir_sandbox_config_disk_get_disk_type(GVirSandboxConfigDisk *config)
{
    GVirSandboxConfigDiskPrivate *priv = config->priv;
    return priv->type;
}


/**
 * gvir_sandbox_config_disk_get_format:
 * @config: (transfer none): the sandbox disk config
 *
 * Retrieves the format property for the custom disk
 *
 * Returns: (transfer none): the format property of disk
 */
GVirConfigDomainDiskFormat gvir_sandbox_config_disk_get_format(GVirSandboxConfigDisk *config)
{
    GVirSandboxConfigDiskPrivate *priv = config->priv;
    return priv->format;
}


/**
 * gvir_sandbox_config_disk_get_tag:
 * @config: (transfer none): the sandbox disk config
 *
 * Retrieves the tag property for the custom disk
 *
 * Returns: (transfer none): the tag property path
 */
const gchar *gvir_sandbox_config_disk_get_tag(GVirSandboxConfigDisk *config)
{
    GVirSandboxConfigDiskPrivate *priv = config->priv;
    return priv->tag;
}


/**
 * gvir_sandbox_config_disk_get_source:
 * @config: (transfer none): the sandbox disk config
 *
 * Retrieves the source property for the custom disk
 *
 * Returns: (transfer none): the source property
 */
const gchar *gvir_sandbox_config_disk_get_source(GVirSandboxConfigDisk *config)
{
    GVirSandboxConfigDiskPrivate *priv = config->priv;
    return priv->source;
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
