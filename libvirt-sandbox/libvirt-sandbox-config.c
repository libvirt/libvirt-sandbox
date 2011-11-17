/*
 * libvirt-sandbox-config.c: libvirt sandbox configuration
 *
 * Copyright (C) 2011 Red Hat
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 *
 * Author: Daniel P. Berrange <berrange@redhat.com>
 */

#include <config.h>
#include <sys/utsname.h>

#include "libvirt-sandbox/libvirt-sandbox.h"

#define GVIR_SANDBOX_CONFIG_GET_PRIVATE(obj)                         \
        (G_TYPE_INSTANCE_GET_PRIVATE((obj), GVIR_SANDBOX_TYPE_CONFIG, GVirSandboxConfigPrivate))

struct _GVirSandboxConfigPrivate
{
    gchar *name;
    gchar *root;
    gchar *arch;
    GList *mounts;

    gchar **command;

    gchar *secType;
    gchar *secLevel;
};

G_DEFINE_TYPE(GVirSandboxConfig, gvir_sandbox_config, G_TYPE_OBJECT);


enum {
    PROP_0,

    PROP_NAME,
    PROP_ROOT,
    PROP_ARCH,

    PROP_SECURITY_TYPE,
    PROP_SECURITY_LEVEL,
};

enum {
    LAST_SIGNAL
};

//static gint signals[LAST_SIGNAL];


static void gvir_sandbox_config_get_property(GObject *object,
                                             guint prop_id,
                                             GValue *value,
                                             GParamSpec *pspec)
{
    GVirSandboxConfig *config = GVIR_SANDBOX_CONFIG(object);
    GVirSandboxConfigPrivate *priv = config->priv;

    switch (prop_id) {
    case PROP_NAME:
        g_value_set_string(value, priv->name);
        break;

    case PROP_ROOT:
        g_value_set_string(value, priv->root);
        break;

    case PROP_ARCH:
        g_value_set_string(value, priv->arch);
        break;

    case PROP_SECURITY_TYPE:
        g_value_set_string(value, priv->secType);
        break;

    case PROP_SECURITY_LEVEL:
        g_value_set_string(value, priv->secLevel);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_config_set_property(GObject *object,
                                             guint prop_id,
                                             const GValue *value,
                                             GParamSpec *pspec)
{
    GVirSandboxConfig *config = GVIR_SANDBOX_CONFIG(object);
    GVirSandboxConfigPrivate *priv = config->priv;

    switch (prop_id) {
    case PROP_NAME:
        g_free(priv->name);
        priv->name = g_value_dup_string(value);
        break;

    case PROP_ROOT:
        g_free(priv->root);
        priv->root = g_value_dup_string(value);
        break;

    case PROP_ARCH:
        g_free(priv->arch);
        priv->arch = g_value_dup_string(value);
        break;

    case PROP_SECURITY_TYPE:
        g_free(priv->secType);
        priv->secType = g_value_dup_string(value);
        break;

    case PROP_SECURITY_LEVEL:
        g_free(priv->secLevel);
        priv->secLevel = g_value_dup_string(value);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_config_finalize(GObject *object)
{
    GVirSandboxConfig *config = GVIR_SANDBOX_CONFIG(object);
    GVirSandboxConfigPrivate *priv = config->priv;

    g_list_foreach(priv->mounts, (GFunc)g_object_unref, NULL);
    g_list_free(priv->mounts);

    g_strfreev(priv->command);
    fprintf(stderr,">>>[%s]\n", priv->name);

    g_free(priv->name);
    g_free(priv->root);
    g_free(priv->arch);
    g_free(priv->secType);
    g_free(priv->secLevel);

    G_OBJECT_CLASS(gvir_sandbox_config_parent_class)->finalize(object);
}


static void gvir_sandbox_config_class_init(GVirSandboxConfigClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = gvir_sandbox_config_finalize;
    object_class->get_property = gvir_sandbox_config_get_property;
    object_class->set_property = gvir_sandbox_config_set_property;

    g_object_class_install_property(object_class,
                                    PROP_NAME,
                                    g_param_spec_string("name",
                                                        "Name",
                                                        "The sandbox name",
                                                        NULL,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_ROOT,
                                    g_param_spec_string("root",
                                                        "Root",
                                                        "The sandbox root",
                                                        NULL,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_ARCH,
                                    g_param_spec_string("arch",
                                                        "Arch",
                                                        "The sandbox architecture",
                                                        NULL,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_SECURITY_TYPE,
                                    g_param_spec_string("security-type",
                                                        "Security type",
                                                        "The SELinux security type",
                                                        NULL,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_SECURITY_LEVEL,
                                    g_param_spec_string("security-level",
                                                        "Security level",
                                                        "The SELinux security level",
                                                        NULL,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_type_class_add_private(klass, sizeof(GVirSandboxConfigPrivate));
}


static void gvir_sandbox_config_init(GVirSandboxConfig *config)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    struct utsname uts;

    uname(&uts);

    priv = config->priv = GVIR_SANDBOX_CONFIG_GET_PRIVATE(config);

    priv->name = g_strdup("sandbox");
    priv->root = g_strdup("/");
    priv->arch = g_strdup(uts.machine);
    priv->secType = g_strdup("svirt_sandbox_t");
}


/**
 * gvir_sandbox_config_new:
 * @name: the sandbox name
 *
 * Create a new console application sandbox configuration
 *
 * Returns: (transfer full): a new sandbox config object
 */
GVirSandboxConfig *gvir_sandbox_config_new(const gchar *name)
{
    return GVIR_SANDBOX_CONFIG(g_object_new(GVIR_SANDBOX_TYPE_CONFIG,
                                            "name", name,
                                            NULL));
}

/**
 * gvir_sandbox_config_get_name:
 * @config: (transfer none): the sandbox config
 *
 * Retrieves the sandbox name
 *
 * Returns: (transfer none): the current name
 */
const gchar *gvir_sandbox_config_get_name(GVirSandboxConfig *config)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    return priv->name;
}


/**
 * gvir_sandbox_config_set_root:
 * @config: (transfer none): the sandbox config
 * @hostdir: (transfer none): the host directory
 *
 * Set the host directory to use as the root for the sandbox. The
 * defualt root is "/".
 */
void gvir_sandbox_config_set_root(GVirSandboxConfig *config, const gchar *hostdir)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    g_free(priv->root);
    priv->root = g_strdup(hostdir);
}

/**
 * gvir_sandbox_config_get_root:
 * @config: (transfer none): the sandbox config
 *
 * Retrieves the sandbox root directory
 *
 * Returns: (transfer none): the current root
 */
const gchar *gvir_sandbox_config_get_root(GVirSandboxConfig *config)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    return priv->root;
}


/**
 * gvir_sandbox_config_set_arch:
 * @config: (transfer none): the sandbox config
 * @arch: (transfer none): the host directory
 *
 * Set the architecture to use in the sandbox. If none is provided,
 * it will default to matching the host architecture.
 */
void gvir_sandbox_config_set_arch(GVirSandboxConfig *config, const gchar *arch)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    g_free(priv->arch);
    priv->arch = g_strdup(arch);
}

/**
 * gvir_sandbox_config_get_arch:
 * @config: (transfer none): the sandbox config
 *
 * Retrieves the sandbox binary architecture
 *
 * Returns: (transfer none): the current architecture
 */
const gchar *gvir_sandbox_config_get_arch(GVirSandboxConfig *config)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    return priv->arch;
}


/**
 * gvir_sandbox_config_add_mount:
 * @config: (transfer none): the sandbox config
 * @mnt: (transfer none): the mount configuration
 *
 * Adds a new custom mount to the sandbox, to override part of the
 * host filesystem
 */
void gvir_sandbox_config_add_mount(GVirSandboxConfig *config,
                                   GVirSandboxConfigMount *mnt)
{
    GVirSandboxConfigPrivate *priv = config->priv;

    g_object_ref(mnt);
    priv->mounts = g_list_append(priv->mounts, mnt);
}


/**
 * gvir_sandbox_config_get_mounts:
 * @config: (transfer none): the sandbox config
 *
 * Retrieves the list of custom mounts in the sandbox
 *
 * Returns: (transfer full) (element-type GVirSandboxConfigMount): the list of mounts
 */
GList *gvir_sandbox_config_get_mounts(GVirSandboxConfig *config)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    g_list_foreach(priv->mounts, (GFunc)g_object_ref, NULL);
    return g_list_copy(priv->mounts);
}


/**
 * gvir_sandbox_config_set_command:
 * @config: (transfer none): the sandbox config
 * @argv: (transfer none)(array zero-terminated=1): the command path and arguments
 * 
 * Set the path of the command to be run and its arguments. The @argv should
 * be a NULL terminated list
 */
void gvir_sandbox_config_set_command(GVirSandboxConfig *config, gchar **argv)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    guint len = g_strv_length(argv);
    size_t i;
    g_strfreev(priv->command);
    priv->command = g_new0(gchar *, len);
    for (i = 0 ; i < len ; i++)
        priv->command[i] = g_strdup(argv[i]);
}


/**
 * gvir_sandbox_config_get_command:
 *
 * Retrieve the sandbox command and arguments
 *
 * Returns: (transfer none)(array zero-terminated=1): the command path and arguments
 */
gchar **gvir_sandbox_config_get_command(GVirSandboxConfig *config)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    return priv->command;
}

/**
 * gvir_sandbox_config_set_security_type:
 * @config: (transfer none): the sandbox config
 * @type: (transfer none): the security level
 *
 * Set the SELinux security type for the sandbox. The default
 * type is "svirt_sandbox_t"
 */
void gvir_sandbox_config_set_security_type(GVirSandboxConfig *config, const gchar *type)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    g_free(priv->secType);
    priv->secType = g_strdup(type);
}

/**
 * gvir_sandbox_config_get_security_type:
 * @config: (transfer none): the sandbox config
 *
 * Retrieve the sandbox security type
 *
 * Returns: (transfer none): the security type
 */
const gchar *gvir_sandbox_config_get_security_type(GVirSandboxConfig *config)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    return priv->secType;
}

/**
 * gvir_sandbox_config_set_security_level:
 * @config: (transfer none): the sandbox config
 * @level: (transfer none): the host directory
 *
 * Set the sandbox security level. By default a dynamic security level
 * is chosen. A static security level must be specified if any
 * custom mounts are added
 */
void gvir_sandbox_config_set_security_level(GVirSandboxConfig *config, const gchar *level)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    g_free(priv->secLevel);
    priv->secLevel = g_strdup(level);
}

/**
 * gvir_sandbox_config_get_security_level:
 * @config: (transfer none): the sandbox config
 *
 * Retrieve the sandbox security level
 *
 * Returns: (transfer none): the security level
 */
const gchar *gvir_sandbox_config_get_security_level(GVirSandboxConfig *config)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    return priv->secLevel;
}

