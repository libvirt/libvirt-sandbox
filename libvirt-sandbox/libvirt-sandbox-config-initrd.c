/*
 * libvirt-sandbox-config-initrd.c: libvirt sandbox configuration
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

#include <config.h>
#include <string.h>

#include "libvirt-sandbox/libvirt-sandbox.h"

/**
 * SECTION: libvirt-sandbox-config-initrd
 * @short_description: Kernel ramdisk configuration details
 * @include: libvirt-sandbox/libvirt-sandbox.h
 *
 * Provides an object to store information about a kernel ramdisk
 *
 * The GVirSandboxConfigInitrd object stores the information required
 * to build a kernel ramdisk to use when booting a virtual machine
 * as a sandbox.
 */

#define GVIR_SANDBOX_CONFIG_INITRD_GET_PRIVATE(obj)                         \
        (G_TYPE_INSTANCE_GET_PRIVATE((obj), GVIR_SANDBOX_TYPE_CONFIG_INITRD, GVirSandboxConfigInitrdPrivate))

struct _GVirSandboxConfigInitrdPrivate
{
    gchar *kver;
    gchar *init;
    gchar *kmoddir;
    GList *modules;
};

G_DEFINE_TYPE(GVirSandboxConfigInitrd, gvir_sandbox_config_initrd, G_TYPE_OBJECT);


enum {
    PROP_0,
    PROP_KVER,
    PROP_KMODDIR,
    PROP_INIT,
};

enum {
    LAST_SIGNAL
};

//static gint signals[LAST_SIGNAL];


static void gvir_sandbox_config_initrd_get_property(GObject *object,
                                                   guint prop_id,
                                                   GValue *value,
                                                   GParamSpec *pspec)
{
    GVirSandboxConfigInitrd *config = GVIR_SANDBOX_CONFIG_INITRD(object);
    GVirSandboxConfigInitrdPrivate *priv = config->priv;

    switch (prop_id) {
    case PROP_KVER:
        g_value_set_string(value, priv->kver);
        break;

    case PROP_KMODDIR:
        g_value_set_string(value, priv->kmoddir);
        break;

    case PROP_INIT:
        g_value_set_string(value, priv->init);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_config_initrd_set_property(GObject *object,
                                                       guint prop_id,
                                                       const GValue *value,
                                                       GParamSpec *pspec)
{
    GVirSandboxConfigInitrd *config = GVIR_SANDBOX_CONFIG_INITRD(object);
    GVirSandboxConfigInitrdPrivate *priv = config->priv;

    switch (prop_id) {
    case PROP_KVER:
        g_free(priv->kver);
        priv->kver = g_value_dup_string(value);
        break;

    case PROP_KMODDIR:
        g_free(priv->kmoddir);
        priv->kmoddir = g_value_dup_string(value);
        break;

    case PROP_INIT:
        g_free(priv->init);
        priv->init = g_value_dup_string(value);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_config_initrd_finalize(GObject *object)
{
    GVirSandboxConfigInitrd *config = GVIR_SANDBOX_CONFIG_INITRD(object);
    GVirSandboxConfigInitrdPrivate *priv = config->priv;

    g_free(priv->init);

    G_OBJECT_CLASS(gvir_sandbox_config_initrd_parent_class)->finalize(object);
}


static void gvir_sandbox_config_initrd_class_init(GVirSandboxConfigInitrdClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = gvir_sandbox_config_initrd_finalize;
    object_class->get_property = gvir_sandbox_config_initrd_get_property;
    object_class->set_property = gvir_sandbox_config_initrd_set_property;

    g_object_class_install_property(object_class,
                                    PROP_KVER,
                                    g_param_spec_string("kver",
                                                        "Kernel version",
                                                        "The host kernel version",
                                                        NULL,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_KMODDIR,
                                    g_param_spec_string("kmoddir",
                                                        "Kmoddir",
                                                        "Kernel modules directory",
                                                        NULL,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_INIT,
                                    g_param_spec_string("init",
                                                        "Init",
                                                        "The host init path",
                                                        NULL,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_type_class_add_private(klass, sizeof(GVirSandboxConfigInitrdPrivate));
}


static void gvir_sandbox_config_initrd_init(GVirSandboxConfigInitrd *config)
{
    GVirSandboxConfigInitrdPrivate *priv = config->priv;
    priv = config->priv = GVIR_SANDBOX_CONFIG_INITRD_GET_PRIVATE(config);
    priv->init = g_strdup(LIBEXECDIR "/libvirt-sandbox-init-qemu");
}


/**
 * gvir_sandbox_config_initrd_new:
 *
 * Create a new initrd config
 *
 * Returns: (transfer full): a new sandbox initrd object
 */
GVirSandboxConfigInitrd *gvir_sandbox_config_initrd_new(void)
{
    return GVIR_SANDBOX_CONFIG_INITRD(g_object_new(GVIR_SANDBOX_TYPE_CONFIG_INITRD,
                                                   NULL));
}



/**
 * gvir_sandbox_config_initrd_set_kver:
 * @config: (transfer none): the sandbox initrd config
 * @version: (transfer none): the kernel version
 *
 * Sets the host kernel version to use for populating the initrd with modules.
 * This defaults to the currently running kernel version
 */
void gvir_sandbox_config_initrd_set_kver(GVirSandboxConfigInitrd *config, const gchar *version)
{
    GVirSandboxConfigInitrdPrivate *priv = config->priv;
    g_free(priv->kver);
    priv->kver = g_strdup(version);
}

/**
 * gvir_sandbox_config_initrd_get_kver:
 * @config: (transfer none): the sandbox initrd config
 *
 * Retrieves the path of the kver binary
 *
 * Returns: (transfer none): the kver binary path
 */
const gchar *gvir_sandbox_config_initrd_get_kver(GVirSandboxConfigInitrd *config)
{
    GVirSandboxConfigInitrdPrivate *priv = config->priv;
    return priv->kver;
}

/**
 * gvir_sandbox_config_initrd_set_kmoddir:
 * @config: (transfer none): the sandbox initrd config
 * @kmoddir: (transfer none): the full path to the kernel modules directory
 *
 * Sets the full path to where the kernel modules will be looked up
 */
void gvir_sandbox_config_initrd_set_kmoddir(GVirSandboxConfigInitrd *config, const gchar *kmoddir)
{
    GVirSandboxConfigInitrdPrivate *priv = config->priv;
    g_free(priv->kmoddir);
    priv->kmoddir = g_strdup(kmoddir);
}

/**
 * gvir_sandbox_config_initrd_get_kmoddir:
 * @config: (transfer none): the full path to the kernel modules directory
 *
 * Retrieves the current kernel modules directory
 *
 * Returns: (transfer none): the full path to the kernel modules directory
 */
const gchar *gvir_sandbox_config_initrd_get_kmoddir(GVirSandboxConfigInitrd *config)
{
    GVirSandboxConfigInitrdPrivate *priv = config->priv;
    return priv->kmoddir;
}


/**
 * gvir_sandbox_config_initrd_set_init:
 * @config: (transfer none): the sandbox initrd config
 * @hostpath: (transfer none): the init binary path
 *
 * Sets the host binary to be used as the init program inside
 * the initrd. This defaults to /usr/bin/libvirt-sandbox-init-qemu
 */
void gvir_sandbox_config_initrd_set_init(GVirSandboxConfigInitrd *config, const gchar *hostpath)
{
    GVirSandboxConfigInitrdPrivate *priv = config->priv;
    g_free(priv->init);
    priv->init = g_strdup(hostpath);
}

/**
 * gvir_sandbox_config_initrd_get_init:
 * @config: (transfer none): the sandbox initrd config
 *
 * Retrieves the path of the init binary
 *
 * Returns: (transfer none): the init binary path
 */
const gchar *gvir_sandbox_config_initrd_get_init(GVirSandboxConfigInitrd *config)
{
    GVirSandboxConfigInitrdPrivate *priv = config->priv;
    return priv->init;
}


/**
 * gvir_sandbox_config_initrd_add_module:
 * @config: (transfer none): the sandbox initrd config
 * @modname: (transfer none): the kernel module name
 *
 * Request that the kernel module @modname is included in the initrd,
 * along with any depedent modules
 */
void gvir_sandbox_config_initrd_add_module(GVirSandboxConfigInitrd *config,
                                           const gchar *modname)
{
    GVirSandboxConfigInitrdPrivate *priv = config->priv;
    priv->modules = g_list_append(priv->modules, g_strdup(modname));
}


/**
 * gvir_sandbox_config_initrd_get_modules:
 * @config: (transfer none): the sandbox initrd config
 *
 * Retrieves the list of all modules
 *
 * Returns: (transfer container)(element-type utf8): the module names
 */
GList *gvir_sandbox_config_initrd_get_modules(GVirSandboxConfigInitrd *config)
{
    GVirSandboxConfigInitrdPrivate *priv = config->priv;
    return g_list_copy(priv->modules);
}
