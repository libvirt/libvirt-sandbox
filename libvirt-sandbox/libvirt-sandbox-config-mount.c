/*
 * libvirt-sandbox-config-mount.c: libvirt sandbox configuration
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
 * SECTION: libvirt-sandbox-config-mount
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

#define GVIR_SANDBOX_CONFIG_MOUNT_GET_PRIVATE(obj)                         \
        (G_TYPE_INSTANCE_GET_PRIVATE((obj), GVIR_SANDBOX_TYPE_CONFIG_MOUNT, GVirSandboxConfigMountPrivate))

struct _GVirSandboxConfigMountPrivate
{
    gchar *target;
    GHashTable *includes;
};

G_DEFINE_ABSTRACT_TYPE(GVirSandboxConfigMount, gvir_sandbox_config_mount, G_TYPE_OBJECT);


enum {
    PROP_0,
    PROP_TARGET,
};

enum {
    LAST_SIGNAL
};

//static gint signals[LAST_SIGNAL];


static void gvir_sandbox_config_mount_get_property(GObject *object,
                                                   guint prop_id,
                                                   GValue *value,
                                                   GParamSpec *pspec)
{
    GVirSandboxConfigMount *config = GVIR_SANDBOX_CONFIG_MOUNT(object);
    GVirSandboxConfigMountPrivate *priv = config->priv;

    switch (prop_id) {
    case PROP_TARGET:
        g_value_set_string(value, priv->target);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_config_mount_set_property(GObject *object,
                                                       guint prop_id,
                                                       const GValue *value,
                                                       GParamSpec *pspec)
{
    GVirSandboxConfigMount *config = GVIR_SANDBOX_CONFIG_MOUNT(object);
    GVirSandboxConfigMountPrivate *priv = config->priv;

    switch (prop_id) {
    case PROP_TARGET:
        g_free(priv->target);
        priv->target = g_value_dup_string(value);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_config_mount_finalize(GObject *object)
{
    GVirSandboxConfigMount *config = GVIR_SANDBOX_CONFIG_MOUNT(object);
    GVirSandboxConfigMountPrivate *priv = config->priv;

    g_free(priv->target);

    G_OBJECT_CLASS(gvir_sandbox_config_mount_parent_class)->finalize(object);
}


static void gvir_sandbox_config_mount_class_init(GVirSandboxConfigMountClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = gvir_sandbox_config_mount_finalize;
    object_class->get_property = gvir_sandbox_config_mount_get_property;
    object_class->set_property = gvir_sandbox_config_mount_set_property;

    g_object_class_install_property(object_class,
                                    PROP_TARGET,
                                    g_param_spec_string("target",
                                                        "Target",
                                                        "The sandbox target directory",
                                                        NULL,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_type_class_add_private(klass, sizeof(GVirSandboxConfigMountPrivate));
}


static void gvir_sandbox_config_mount_init(GVirSandboxConfigMount *config)
{
    GVirSandboxConfigMountPrivate *priv = config->priv;
    priv = config->priv = GVIR_SANDBOX_CONFIG_MOUNT_GET_PRIVATE(config);

    priv->includes = g_hash_table_new_full(g_str_hash,
                                           g_str_equal,
                                           g_free,
                                           g_free);
}


/**
 * gvir_sandbox_config_mount_get_target:
 * @config: (transfer none): the sandbox mount config
 *
 * Retrieves the target directory for the custom mount
 *
 * Returns: (transfer none): the target directory path
 */
const gchar *gvir_sandbox_config_mount_get_target(GVirSandboxConfigMount *config)
{
    GVirSandboxConfigMountPrivate *priv = config->priv;
    return priv->target;
}


/**
 * gvir_sandbox_config_mount_add_include:
 * @config: (transfer none): the sandbox mount config
 * @srcpath: (transfer none): a file on the host
 * @dstpath: (transfer none): a path within the mount
 *
 * Request that the file @srcpath from the host OS is to be copied
 * to @dstpath, relative to the @target path in the sandbox.
 */
void gvir_sandbox_config_mount_add_include(GVirSandboxConfigMount *config,
                                           const gchar *srcpath, const gchar *dstpath)
{
    GVirSandboxConfigMountPrivate *priv = config->priv;
    g_hash_table_insert(priv->includes, g_strdup(dstpath), g_strdup(srcpath));
}


/**
 * gvir_sandbox_config_mount_get_includes:
 * @config: (transfer none): the sandbox mount config
 *
 * Retrieves the list of all include files
 *
 * Returns: (transfer none)(element-type filename filename): the include files
 */
GHashTable *gvir_sandbox_config_mount_get_includes(GVirSandboxConfigMount *config)
{
    GVirSandboxConfigMountPrivate *priv = config->priv;
    return priv->includes;
}
