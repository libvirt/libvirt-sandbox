/*
 * libvirt-sandbox-builder-container.c: libvirt sandbox configuration
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 *
 * Author: Daniel P. Berrange <berrange@redhat.com>
 */

#include <config.h>
#include <string.h>

#include "libvirt-sandbox/libvirt-sandbox.h"

/**
 * SECTION: libvirt-sandbox-builder-container
 * @short_description: Sandbox container construction
 * @include: libvirt-sandbox/libvirt-sandbox.h
 * @see_also: #GVirSandboxBuilder, #GVirSandboxBuilderMachine
 *
 * Provides an object for creating sandboxes using container virtualization
 *
 * The GVirSandboxBuilderContainer object provides a way to builder sandboxes
 * using OS container virtualization technologies such as LXC.
 */

#define GVIR_SANDBOX_BUILDER_CONTAINER_GET_PRIVATE(obj)                         \
        (G_TYPE_INSTANCE_GET_PRIVATE((obj), GVIR_SANDBOX_TYPE_BUILDER_CONTAINER, GVirSandboxBuilderContainerPrivate))

struct _GVirSandboxBuilderContainerPrivate
{
    gboolean unused;
};

G_DEFINE_TYPE(GVirSandboxBuilderContainer, gvir_sandbox_builder_container, GVIR_SANDBOX_TYPE_BUILDER);


enum {
    PROP_0,
};

enum {
    LAST_SIGNAL
};

//static gint signals[LAST_SIGNAL];

#define GVIR_SANDBOX_BUILDER_CONTAINER_ERROR gvir_sandbox_builder_container_error_quark()

static GQuark
gvir_sandbox_builder_container_error_quark(void)
{
    return g_quark_from_static_string("gvir-sandbox-builder-container");
}


static void gvir_sandbox_builder_container_get_property(GObject *object,
                                                      guint prop_id,
                                                      GValue *value G_GNUC_UNUSED,
                                                      GParamSpec *pspec)
{
#if 0
    GVirSandboxBuilderContainer *builder = GVIR_SANDBOX_BUILDER_CONTAINER(object);
    GVirSandboxBuilderContainerPrivate *priv = builder->priv;
#endif

    switch (prop_id) {

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_builder_container_set_property(GObject *object,
                                                      guint prop_id,
                                                      const GValue *value G_GNUC_UNUSED,
                                                      GParamSpec *pspec)
{
#if 0
    GVirSandboxBuilderContainer *builder = GVIR_SANDBOX_BUILDER_CONTAINER(object);
    GVirSandboxBuilderContainerPrivate *priv = builder->priv;
#endif

    switch (prop_id) {

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_builder_container_finalize(GObject *object)
{
#if 0
    GVirSandboxBuilderContainer *builder = GVIR_SANDBOX_BUILDER_CONTAINER(object);
    GVirSandboxBuilderContainerPrivate *priv = builder->priv;
#endif

    G_OBJECT_CLASS(gvir_sandbox_builder_container_parent_class)->finalize(object);
}


static gchar *gvir_sandbox_builder_container_cmdline(GVirSandboxConfig *config)
{
    GString *str = g_string_new("");
    gchar **argv, **tmp;
    gchar *cmdrawstr;
    gchar *cmdb64str;
    size_t len = 0, offset = 0;
    gchar *lenstr;
    gchar *ret;
    gsize prefix = 0;

    argv = gvir_sandbox_config_get_command(config);

    tmp = argv;
    while (*tmp) {
        len += strlen(*tmp) + 1;
        tmp++;
    }

    lenstr = g_strdup_printf("%zu", len);
    prefix = strlen(lenstr);
    cmdrawstr = g_new0(gchar, prefix + 1 + len);

    memcpy(cmdrawstr, lenstr, prefix);
    cmdrawstr[prefix] = ':';
    offset += prefix + 1;

    tmp = argv;
    while (*tmp) {
        size_t tlen = strlen(*tmp);
        memcpy(cmdrawstr + offset, *tmp, tlen + 1);
        offset += tlen + 1;
        tmp++;
    }

    cmdb64str = g_base64_encode((guchar*)cmdrawstr, prefix + 1 + len);
    g_free(cmdrawstr);

    /* First sandbox specific args */
    g_string_append_printf(str, "sandbox-cmd=%s", cmdb64str);
    g_free(cmdb64str);

    if (gvir_sandbox_config_get_tty(config))
        g_string_append(str, " sandbox-isatty");

    g_string_append_printf(str, " sandbox-uid=%u", gvir_sandbox_config_get_userid(config));
    g_string_append_printf(str, " sandbox-gid=%u", gvir_sandbox_config_get_groupid(config));
    g_string_append_printf(str, " sandbox-user=%s", gvir_sandbox_config_get_username(config));
    g_string_append_printf(str, " sandbox-home=%s", gvir_sandbox_config_get_homedir(config));

    ret = str->str;
    g_string_free(str, FALSE);
    return ret;
}


static gboolean gvir_sandbox_builder_container_construct_basic(GVirSandboxBuilder *builder,
                                                               GVirSandboxConfig *config,
                                                               GVirSandboxCleaner *cleaner,
                                                               GVirConfigDomain *domain,
                                                               GError **error)
{
    if (!GVIR_SANDBOX_BUILDER_CLASS(gvir_sandbox_builder_container_parent_class)->
        construct_basic(builder, config, cleaner, domain, error))
        return FALSE;

    gvir_config_domain_set_virt_type(domain,
                                     GVIR_CONFIG_DOMAIN_VIRT_LXC);

    return TRUE;
}


static gboolean gvir_sandbox_builder_container_construct_os(GVirSandboxBuilder *builder,
                                                            GVirSandboxConfig *config,
                                                            GVirSandboxCleaner *cleaner,
                                                            GVirConfigDomain *domain,
                                                            GError **error)
{
    gchar *cmdline;
    GVirConfigDomainOs *os;

    if (!GVIR_SANDBOX_BUILDER_CLASS(gvir_sandbox_builder_container_parent_class)->
        construct_os(builder, config, cleaner, domain, error))
        return FALSE;

    cmdline = gvir_sandbox_builder_container_cmdline(config);

    os = gvir_config_domain_os_new();
    gvir_config_domain_os_set_os_type(os,
                                      GVIR_CONFIG_DOMAIN_OS_TYPE_EXE);
    gvir_config_domain_os_set_arch(os,
                                   gvir_sandbox_config_get_arch(config));
    gvir_config_domain_os_set_init(os,
                                   LIBEXECDIR "/libvirt-sandbox-init-lxc");
    gvir_config_domain_os_set_cmdline(os, cmdline);

    gvir_config_domain_set_os(domain, os);

    g_free(cmdline);

    return TRUE;
}


static gboolean gvir_sandbox_builder_container_construct_features(GVirSandboxBuilder *builder,
                                                                  GVirSandboxConfig *config,
                                                                  GVirSandboxCleaner *cleaner,
                                                                  GVirConfigDomain *domain,
                                                                  GError **error)
{
    if (!GVIR_SANDBOX_BUILDER_CLASS(gvir_sandbox_builder_container_parent_class)->
        construct_features(builder, config, cleaner, domain, error))
        return FALSE;

    return TRUE;
}

static gboolean gvir_sandbox_builder_container_construct_devices(GVirSandboxBuilder *builder,
                                                                 GVirSandboxConfig *config,
                                                                 GVirSandboxCleaner *cleaner,
                                                                 GVirConfigDomain *domain,
                                                                 GError **error)
{
    GVirConfigDomainFilesys *fs;
    GVirConfigDomainMemballoon *ball;
    GList *tmp = NULL, *mounts = NULL;

    if (!GVIR_SANDBOX_BUILDER_CLASS(gvir_sandbox_builder_container_parent_class)->
        construct_devices(builder, config, cleaner, domain, error))
        return FALSE;

    fs = gvir_config_domain_filesys_new();
    gvir_config_domain_filesys_set_type(fs, GVIR_CONFIG_DOMAIN_FILESYS_MOUNT);
    gvir_config_domain_filesys_set_access_type(fs, GVIR_CONFIG_DOMAIN_FILESYS_ACCESS_PASSTHROUGH);
    gvir_config_domain_filesys_set_source(fs,
                                          gvir_sandbox_config_get_root(config));
    gvir_config_domain_filesys_set_target(fs, "/");
    gvir_config_domain_filesys_set_readonly(fs, TRUE);

    gvir_config_domain_add_device(domain,
                                  GVIR_CONFIG_DOMAIN_DEVICE(fs));
    g_object_unref(fs);

    tmp = mounts = gvir_sandbox_config_get_host_mounts(config);
    while (tmp) {
        GVirSandboxConfigMount *mconfig = tmp->data;

        fs = gvir_config_domain_filesys_new();
        gvir_config_domain_filesys_set_type(fs, GVIR_CONFIG_DOMAIN_FILESYS_MOUNT);
        gvir_config_domain_filesys_set_access_type(fs, GVIR_CONFIG_DOMAIN_FILESYS_ACCESS_PASSTHROUGH);
        gvir_config_domain_filesys_set_source(fs,
                                              gvir_sandbox_config_mount_get_root(mconfig));
        gvir_config_domain_filesys_set_target(fs,
                                              gvir_sandbox_config_mount_get_target(mconfig));

        gvir_config_domain_add_device(domain,
                                      GVIR_CONFIG_DOMAIN_DEVICE(fs));
        g_object_unref(fs);

        tmp = tmp->next;
    }
    g_list_foreach(mounts, (GFunc)g_object_unref, NULL);
    g_list_free(mounts);

    ball = gvir_config_domain_memballoon_new();
    gvir_config_domain_memballoon_set_model(ball,
                                            GVIR_CONFIG_DOMAIN_MEMBALLOON_MODEL_NONE);
    gvir_config_domain_add_device(domain,
                                  GVIR_CONFIG_DOMAIN_DEVICE(ball));
    g_object_unref(ball);


    if (GVIR_SANDBOX_IS_CONFIG_GRAPHICAL(config)) {
        g_set_error(error, GVIR_SANDBOX_BUILDER_CONTAINER_ERROR, 0,
                    "%s", "Graphical sandboxes are not supported for containers");
        return FALSE;
    }

    return TRUE;
}


static void gvir_sandbox_builder_container_class_init(GVirSandboxBuilderContainerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    GVirSandboxBuilderClass *builder_class = GVIR_SANDBOX_BUILDER_CLASS(klass);

    object_class->finalize = gvir_sandbox_builder_container_finalize;
    object_class->get_property = gvir_sandbox_builder_container_get_property;
    object_class->set_property = gvir_sandbox_builder_container_set_property;

    builder_class->construct_basic = gvir_sandbox_builder_container_construct_basic;
    builder_class->construct_os = gvir_sandbox_builder_container_construct_os;
    builder_class->construct_features = gvir_sandbox_builder_container_construct_features;
    builder_class->construct_devices = gvir_sandbox_builder_container_construct_devices;

    g_type_class_add_private(klass, sizeof(GVirSandboxBuilderContainerPrivate));
}


static void gvir_sandbox_builder_container_init(GVirSandboxBuilderContainer *builder)
{
    builder->priv = GVIR_SANDBOX_BUILDER_CONTAINER_GET_PRIVATE(builder);
}


/**
 * gvir_sandbox_builder_container_new:
 * @connection: (transfer none): the connection
 *
 * Create a new graphical application sandbox builderuration
 *
 * Returns: (transfer full): a new graphical sandbox builder object
 */
GVirSandboxBuilderContainer *gvir_sandbox_builder_container_new(GVirConnection *connection)
{
    return GVIR_SANDBOX_BUILDER_CONTAINER(g_object_new(GVIR_SANDBOX_TYPE_BUILDER_CONTAINER,
                                                       "connection", connection,
                                                       NULL));
}
