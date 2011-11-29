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
#include <string.h>

#include "libvirt-sandbox/libvirt-sandbox.h"

/**
 * SECTION: libvirt-sandbox-config
 * @short_description: Basic sandbox configuration details
 * @include: libvirt-sandbox/libvirt-sandbox.h
 * @see_also: #GVirSandboxConfigGraphical
 *
 * Provides a base object to store configurations for the application sandbox
 *
 * The GVirSandboxConfig object stores the basic information required to
 * create application sandboxes with a simple text based console. 
 */

#define GVIR_SANDBOX_CONFIG_GET_PRIVATE(obj)                         \
        (G_TYPE_INSTANCE_GET_PRIVATE((obj), GVIR_SANDBOX_TYPE_CONFIG, GVirSandboxConfigPrivate))

struct _GVirSandboxConfigPrivate
{
    gchar *name;
    gchar *root;
    gchar *arch;
    gboolean tty;

    guint uid;
    guint gid;
    gchar *username;
    gchar *homedir;

    GList *mounts;

    gchar **command;

    gchar *secLabel;
    gboolean secDynamic;
};

G_DEFINE_TYPE(GVirSandboxConfig, gvir_sandbox_config, G_TYPE_OBJECT);


enum {
    PROP_0,

    PROP_NAME,
    PROP_ROOT,
    PROP_ARCH,
    PROP_TTY,

    PROP_UID,
    PROP_GID,
    PROP_USERNAME,
    PROP_HOMEDIR,

    PROP_SECURITY_LABEL,
    PROP_SECURITY_DYNAMIC,
};

enum {
    LAST_SIGNAL
};

//static gint signals[LAST_SIGNAL];

#define GVIR_SANDBOX_CONFIG_ERROR gvir_sandbox_config_error_quark()

static GQuark
gvir_sandbox_config_error_quark(void)
{
    return g_quark_from_static_string("gvir-sandbox-config");
}

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

    case PROP_TTY:
        g_value_set_boolean(value, priv->tty);
        break;

    case PROP_UID:
        g_value_set_uint(value, priv->uid);
        break;

    case PROP_GID:
        g_value_set_uint(value, priv->gid);
        break;

    case PROP_USERNAME:
        g_value_set_string(value, priv->username);
        break;

    case PROP_HOMEDIR:
        g_value_set_string(value, priv->homedir);
        break;

    case PROP_SECURITY_LABEL:
        g_value_set_string(value, priv->secLabel);
        break;

    case PROP_SECURITY_DYNAMIC:
        g_value_set_boolean(value, priv->secDynamic);
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

    case PROP_TTY:
        priv->tty = g_value_get_boolean(value);
        break;

    case PROP_UID:
        priv->uid = g_value_get_uint(value);
        break;

    case PROP_GID:
        priv->gid = g_value_get_uint(value);
        break;

    case PROP_USERNAME:
        g_free(priv->username);
        priv->username = g_value_dup_string(value);
        break;

    case PROP_HOMEDIR:
        g_free(priv->homedir);
        priv->homedir = g_value_dup_string(value);
        break;

    case PROP_SECURITY_LABEL:
        g_free(priv->secLabel);
        priv->secLabel = g_value_dup_string(value);
        break;

    case PROP_SECURITY_DYNAMIC:
        priv->secDynamic = g_value_get_boolean(value);
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

    g_free(priv->name);
    g_free(priv->root);
    g_free(priv->arch);
    g_free(priv->secLabel);

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
                                    PROP_TTY,
                                    g_param_spec_string("tty",
                                                        "TTY",
                                                        "TTY",
                                                        FALSE,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_UID,
                                    g_param_spec_uint("uid",
                                                      "UID",
                                                      "The user ID",
                                                      0,
                                                      G_MAXUINT,
                                                      geteuid(),
                                                      G_PARAM_READABLE |
                                                      G_PARAM_WRITABLE |
                                                      G_PARAM_STATIC_NAME |
                                                      G_PARAM_STATIC_NICK |
                                                      G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_GID,
                                    g_param_spec_uint("gid",
                                                      "GID",
                                                      "The group ID",
                                                      0,
                                                      G_MAXUINT,
                                                      getegid(),
                                                      G_PARAM_READABLE |
                                                      G_PARAM_WRITABLE |
                                                      G_PARAM_STATIC_NAME |
                                                      G_PARAM_STATIC_NICK |
                                                      G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_USERNAME,
                                    g_param_spec_string("username",
                                                        "Username",
                                                        "The username",
                                                        g_get_user_name(),
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_HOMEDIR,
                                    g_param_spec_string("homedir",
                                                        "Homedir",
                                                        "The home directory",
                                                        g_get_home_dir(),
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_SECURITY_LABEL,
                                    g_param_spec_string("security-label",
                                                        "Security label",
                                                        "The security label",
                                                        NULL,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_SECURITY_DYNAMIC,
                                    g_param_spec_boolean("security-dynamic",
                                                         "Security dynamic",
                                                         "The security mode",
                                                         TRUE,
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
    priv->secLabel = g_strdup("system_u:system_r:svirt_t:s0:c0.c1023");

    priv->command = g_new0(gchar *, 2);
    priv->command[0] = g_strdup("/bin/sh");
    priv->command[1] = NULL;

    priv->uid = geteuid();
    priv->gid = getegid();
    priv->username = g_strdup(g_get_user_name());
    priv->homedir = g_strdup(g_get_home_dir());
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
 * gvir_sandbox_config_set_tty:
 * @config: (transfer none): the sandbox config
 * @tty: (transfer none): true if the container should have a tty
 *
 * Set whether the container console should have a interactive tty.
 */
void gvir_sandbox_config_set_tty(GVirSandboxConfig *config, gboolean tty)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    priv->tty = tty;
}


/**
 * gvir_sandbox_config_get_tty:
 * @config: (transfer none): the sandbox config
 *
 * Retrieves the sandbox tty flag
 *
 * Returns: (transfer none): the tty flag
 */
gboolean gvir_sandbox_config_get_tty(GVirSandboxConfig *config)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    return priv->tty;
}


/**
 * gvir_sandbox_config_set_userid:
 * @config: (transfer none): the sandbox config
 * @uid: (transfer none): the sandbox user ID
 *
 * Set the user ID to invoke the sandbox application under.
 * Defaults to the user ID of the current program.
 */
void gvir_sandbox_config_set_userid(GVirSandboxConfig *config, guint uid)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    priv->uid = uid;
}


/**
 * gvir_sandbox_config_get_userid:
 * @config: (transfer none): the sandbox config
 *
 * Get the user ID to invoke the sandbox application under.
 *
 * Returns: (transfer none): the user ID
 */
guint gvir_sandbox_config_get_userid(GVirSandboxConfig *config)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    return priv->uid;
}


/**
 * gvir_sandbox_config_set_groupid:
 * @config: (transfer none): the sandbox config
 * @gid: (transfer none): the sandbox group ID
 *
 * Set the group ID to invoke the sandbox application under.
 * Defaults to the group ID of the current program.
 */
void gvir_sandbox_config_set_groupid(GVirSandboxConfig *config, guint gid)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    priv->gid = gid;
}


/**
 * gvir_sandbox_config_get_groupid:
 * @config: (transfer none): the sandbox config
 *
 * Get the group ID to invoke the sandbox application under.
 *
 * Returns: (transfer none): the group ID
 */
guint gvir_sandbox_config_get_groupid(GVirSandboxConfig *config)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    return priv->gid;
}


/**
 * gvir_sandbox_config_set_username:
 * @config: (transfer none): the sandbox config
 * @username: (transfer none): the sandbox user name
 *
 * Set the user name associated with the sandbox user ID.
 * Defaults to the user name of the current program.
 */
void gvir_sandbox_config_set_username(GVirSandboxConfig *config, const gchar *username)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    g_free(priv->username);
    priv->username = g_strdup(username);
}


/**
 * gvir_sandbox_config_get_username:
 * @config: (transfer none): the sandbox config
 *
 * Get the user name to invoke the sandbox application under.
 *
 * Returns: (transfer none): the user name
 */
const gchar *gvir_sandbox_config_get_username(GVirSandboxConfig *config)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    return priv->username;
}


/**
 * gvir_sandbox_config_set_homedir:
 * @config: (transfer none): the sandbox config
 * @homedir: (transfer none): the sandbox home directory
 *
 * Set the home directory associated with the sandbox user ID.
 * Defaults to the home directory of the current program.
 */
void gvir_sandbox_config_set_homedir(GVirSandboxConfig *config, const gchar *homedir)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    g_free(priv->homedir);
    priv->homedir = g_strdup(homedir);
}


/**
 * gvir_sandbox_config_get_homedir:
 * @config: (transfer none): the sandbox config
 *
 * Get the home directory associated with the sandbox user ID
 *
 * Returns: (transfer none): the home directory
 */
const gchar *gvir_sandbox_config_get_homedir(GVirSandboxConfig *config)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    return priv->homedir;
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
 * gvir_sandbox_config_find_mount:
 * @config: (transfer none): the sandbox config
 * @target: the guest target path
 *
 * Finds the #GVirSandboxCOnfigMount object corresponding to a guest
 * path of @target.
 *
 * Returns: (transfer full)(allow-none): a mount object or NULL
 */
GVirSandboxConfigMount *gvir_sandbox_config_find_mount(GVirSandboxConfig *config,
                                                       const gchar *target)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    GList *tmp = priv->mounts;

    while (tmp) {
        GVirSandboxConfigMount *mnt = GVIR_SANDBOX_CONFIG_MOUNT(tmp->data);
        if (g_str_equal(gvir_sandbox_config_mount_get_target(mnt), target)) {
            return g_object_ref(mnt);
        }
        tmp = tmp->next;
    }
    return NULL;
}


/**
 * gvir_sandbox_config_set_mounts:
 * @config: (transfer none): the sandbox config
 * @mounts: (transfer none)(array zero-terminate=1): the list of mounts
 *
 * Parses @mounts whose elements are in the format
 * GUEST-TARGET=ROOT-PATH, creating #GVirSandboxConfigMount
 * instances for each element.
 */
void gvir_sandbox_config_add_mount_strv(GVirSandboxConfig *config,
                                        gchar **mounts)
{
    gsize i = 0;
    while (mounts && mounts[i]) {
        const gchar *guest = mounts[i];
        const gchar *host = NULL;
        GVirSandboxConfigMount *mnt;
        gchar *tmp;

        if ((tmp = strchr(mounts[i], '=')) != NULL) {
            *tmp = '\0';
            host = tmp + 1;
        }

        mnt = gvir_sandbox_config_mount_new(guest);
        gvir_sandbox_config_mount_set_root(mnt, host);

        gvir_sandbox_config_add_mount(config, mnt);
        g_object_unref(mnt);

        i++;
    }
}


gboolean gvir_sandbox_config_add_include_strv(GVirSandboxConfig *config,
                                              gchar **includes,
                                              GError **error)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    gsize i = 0;

    while (includes && includes[i]) {
        const gchar *guest = NULL;
        const gchar *host = includes[i];
        GVirSandboxConfigMount *mnt = NULL;
        GList *mnts = NULL;
        gchar *tmp;

        if ((tmp = strchr(includes[i], '=')) != NULL) {
            *tmp = '\0';
            guest = tmp + 1;
        }

        mnts = priv->mounts;
        while (mnts) {
            mnt = GVIR_SANDBOX_CONFIG_MOUNT(mnts->data);
            const gchar *target = gvir_sandbox_config_mount_get_target(mnt);
            if (g_str_has_prefix(host, target)) {
                if (!guest)
                    guest = host + strlen(target);
                break;
            }
            mnt = NULL;
            mnts = mnts->next;
        }
        if (!mnt) {
            g_set_error(error, GVIR_SANDBOX_CONFIG_ERROR, 0,
                        "No mount with a prefix under %s", host);
            return FALSE;
        }

        gvir_sandbox_config_mount_add_include(mnt, host, guest);

        i++;
    }

    return TRUE;
}


gboolean gvir_sandbox_config_add_include_file(GVirSandboxConfig *config,
                                              gchar *includefile,
                                              GError **error)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    GFile *file = g_file_new_for_path(includefile);
    GFileInputStream *is = NULL;
    GDataInputStream *dis = NULL;
    gboolean ret = FALSE;
    gchar *line = NULL;

    if (!(is = g_file_read(file, NULL, error)))
        goto cleanup;

    dis = g_data_input_stream_new(G_INPUT_STREAM(is));

    while ((line = g_data_input_stream_read_line(dis,
                                                 NULL,
                                                 NULL,
                                                 error))) {
        const gchar *guest = NULL;
        const gchar *host = line;
        GVirSandboxConfigMount *mnt = NULL;
        GList *mnts = NULL;
        gchar *tmp;

        if ((tmp = strchr(line, '=')) != NULL) {
            *tmp = '\0';
            guest = tmp + 1;
        }

        mnts = priv->mounts;
        while (mnts) {
            mnt = GVIR_SANDBOX_CONFIG_MOUNT(mnts->data);
            const gchar *target = gvir_sandbox_config_mount_get_target(mnt);
            if (g_str_has_prefix(host, target)) {
                if (!guest)
                    guest = host + strlen(target);
                break;
            }
            mnt = NULL;
            mnts = mnts->next;
        }
        if (!mnt) {
            g_set_error(error, GVIR_SANDBOX_CONFIG_ERROR, 0,
                        "No mount with a prefix under %s", host);
            goto cleanup;
        }

        gvir_sandbox_config_mount_add_include(mnt, host, guest);
    }

    if (error && *error)
        goto cleanup;

    ret = TRUE;
cleanup:
    if (dis)
        g_object_unref(dis);
    g_object_unref(file);
    return ret;
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
    priv->command = g_new0(gchar *, len + 1);
    for (i = 0 ; i < len ; i++)
        priv->command[i] = g_strdup(argv[i]);
    priv->command[len] = NULL;
}


/**
 * gvir_sandbox_config_get_command:
 * @config: (transfer none): the sandbox config
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
 * gvir_sandbox_config_set_security_label:
 * @config: (transfer none): the sandbox config
 * @label: (transfer none): the host directory
 *
 * Set the sandbox security label. By default a dynamic security label
 * is chosen. A static security label must be specified if any
 * custom mounts are added
 */
void gvir_sandbox_config_set_security_label(GVirSandboxConfig *config, const gchar *label)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    g_free(priv->secLabel);
    priv->secLabel = g_strdup(label);
}

/**
 * gvir_sandbox_config_get_security_label:
 * @config: (transfer none): the sandbox config
 *
 * Retrieve the sandbox security label
 *
 * Returns: (transfer none): the security label
 */
const gchar *gvir_sandbox_config_get_security_label(GVirSandboxConfig *config)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    return priv->secLabel;
}


/**
 * gvir_sandbox_config_set_security_dynamic:
 * @config: (transfer none): the sandbox config
 * @dynamic: (transfer none): the security mode
 *
 * Set the SELinux security dynamic for the sandbox. The default
 * dynamic is "svirt_sandbox_t"
 */
void gvir_sandbox_config_set_security_dynamic(GVirSandboxConfig *config, gboolean dynamic)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    priv->secDynamic = dynamic;
}

/**
 * gvir_sandbox_config_get_security_dynamic:
 * @config: (transfer none): the sandbox config
 *
 * Retrieve the sandbox security mode
 *
 * Returns: (transfer none): the security mode
 */
gboolean gvir_sandbox_config_get_security_dynamic(GVirSandboxConfig *config)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    return priv->secDynamic;
}


gboolean gvir_sandbox_config_set_security_opts(GVirSandboxConfig *config,
                                               const gchar *optstr,
                                               GError **error)
{
    gchar **opts = g_strsplit(optstr, ",", 0);
    gsize i = 0;

    while (opts[i]) {
        gchar *name = opts[i];
        gchar *value = strchr(name, '=');

        if (strncmp(name, "label=", 5) == 0) {
            gvir_sandbox_config_set_security_label(config, value);
        } else if (g_str_equal(name, "dynamic")) {
            gvir_sandbox_config_set_security_dynamic(config, TRUE);
        } else if (g_str_equal(name, "static")) {
            gvir_sandbox_config_set_security_dynamic(config, FALSE);
        } else {
            g_set_error(error, GVIR_SANDBOX_CONFIG_ERROR, 0,
                        "Unknown security option '%s'", name);
            return FALSE;
        }
    }
    return TRUE;
}
