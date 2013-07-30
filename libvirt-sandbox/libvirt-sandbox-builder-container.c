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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Daniel P. Berrange <berrange@redhat.com>
 */

#include <config.h>
#include <string.h>

#include <glib/gi18n.h>

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

#if 0
#define GVIR_SANDBOX_BUILDER_CONTAINER_ERROR gvir_sandbox_builder_container_error_quark()

static GQuark
gvir_sandbox_builder_container_error_quark(void)
{
    return g_quark_from_static_string("gvir-sandbox-builder-container");
}
#endif

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


static gchar *gvir_sandbox_builder_container_cmdline(GVirSandboxConfig *config G_GNUC_UNUSED)
{
    GString *str = g_string_new("");
    gchar *ret;
    gchar *tmp;

    /* Now kernel args */
    if (getenv("LIBVIRT_SANDBOX_DEBUG") &&
        g_str_equal(getenv("LIBVIRT_SANDBOX_DEBUG"), "2"))
        g_string_append(str, "debug");

    if ((tmp = getenv("LIBVIRT_SANDBOX_STRACE"))) {
        g_string_append(str, " strace=");
        g_string_append(str, tmp);
    }

    ret = str->str;
    g_string_free(str, FALSE);
    return ret;
}


static gboolean gvir_sandbox_builder_container_construct_basic(GVirSandboxBuilder *builder,
                                                               GVirSandboxConfig *config,
                                                               const gchar *statedir,
                                                               GVirConfigDomain *domain,
                                                               GError **error)
{
    gchar **features;

    if (!GVIR_SANDBOX_BUILDER_CLASS(gvir_sandbox_builder_container_parent_class)->
        construct_basic(builder, config, statedir, domain, error))
        return FALSE;

    gvir_config_domain_set_virt_type(domain,
                                     GVIR_CONFIG_DOMAIN_VIRT_LXC);

    features = g_new0(gchar *, 2);
    features[0] = g_strdup("privnet");
    gvir_config_domain_set_features(domain, features);
    g_strfreev(features);

    return TRUE;
}


static gboolean gvir_sandbox_builder_container_construct_os(GVirSandboxBuilder *builder,
                                                            GVirSandboxConfig *config,
                                                            const gchar *statedir,
                                                            GVirConfigDomain *domain,
                                                            GError **error)
{
    gchar *cmdline = NULL;
    GVirConfigDomainOs *os;

    if (!GVIR_SANDBOX_BUILDER_CLASS(gvir_sandbox_builder_container_parent_class)->
        construct_os(builder, config, statedir, domain, error))
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
                                                                  const gchar *statedir,
                                                                  GVirConfigDomain *domain,
                                                                  GError **error)
{
    if (!GVIR_SANDBOX_BUILDER_CLASS(gvir_sandbox_builder_container_parent_class)->
        construct_features(builder, config, statedir, domain, error))
        return FALSE;

    return TRUE;
}

static gboolean gvir_sandbox_builder_container_construct_devices(GVirSandboxBuilder *builder,
                                                                 GVirSandboxConfig *config,
                                                                 const gchar *statedir,
                                                                 GVirConfigDomain *domain,
                                                                 GError **error)
{
    GVirConfigDomainFilesys *fs;
    GVirConfigDomainInterfaceNetwork *iface;
    GVirConfigDomainConsole *con;
    GVirConfigDomainChardevSourcePty *src;
    GList *tmp = NULL, *mounts = NULL, *networks = NULL;
    gchar *configdir = g_strdup_printf("%s/config", statedir);
    gboolean ret = FALSE;

    if (!GVIR_SANDBOX_BUILDER_CLASS(gvir_sandbox_builder_container_parent_class)->
        construct_devices(builder, config, statedir, domain, error))
        goto cleanup;

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



    fs = gvir_config_domain_filesys_new();
    gvir_config_domain_filesys_set_type(fs, GVIR_CONFIG_DOMAIN_FILESYS_MOUNT);
    gvir_config_domain_filesys_set_access_type(fs, GVIR_CONFIG_DOMAIN_FILESYS_ACCESS_PASSTHROUGH);
    gvir_config_domain_filesys_set_source(fs, configdir);
    gvir_config_domain_filesys_set_target(fs, SANDBOXCONFIGDIR);
    gvir_config_domain_filesys_set_readonly(fs, TRUE);

    gvir_config_domain_add_device(domain,
                                  GVIR_CONFIG_DOMAIN_DEVICE(fs));
    g_object_unref(fs);



    tmp = mounts = gvir_sandbox_config_get_mounts(config);
    while (tmp) {
        GVirSandboxConfigMount *mconfig = GVIR_SANDBOX_CONFIG_MOUNT(tmp->data);

        if (GVIR_SANDBOX_IS_CONFIG_MOUNT_HOST_BIND(mconfig)) {
            GVirSandboxConfigMountFile *mfile = GVIR_SANDBOX_CONFIG_MOUNT_FILE(mconfig);

            fs = gvir_config_domain_filesys_new();
            gvir_config_domain_filesys_set_type(fs, GVIR_CONFIG_DOMAIN_FILESYS_MOUNT);
            gvir_config_domain_filesys_set_access_type(fs, GVIR_CONFIG_DOMAIN_FILESYS_ACCESS_PASSTHROUGH);
            gvir_config_domain_filesys_set_source(fs,
                                                  gvir_sandbox_config_mount_file_get_source(mfile));
            gvir_config_domain_filesys_set_target(fs,
                                                  gvir_sandbox_config_mount_get_target(mconfig));

            gvir_config_domain_add_device(domain,
                                          GVIR_CONFIG_DOMAIN_DEVICE(fs));
            g_object_unref(fs);
        } else if (GVIR_SANDBOX_IS_CONFIG_MOUNT_HOST_IMAGE(mconfig)) {
            GVirSandboxConfigMountFile *mfile = GVIR_SANDBOX_CONFIG_MOUNT_FILE(mconfig);

            fs = gvir_config_domain_filesys_new();
            gvir_config_domain_filesys_set_type(fs, GVIR_CONFIG_DOMAIN_FILESYS_FILE);
            gvir_config_domain_filesys_set_access_type(fs, GVIR_CONFIG_DOMAIN_FILESYS_ACCESS_PASSTHROUGH);
            gvir_config_domain_filesys_set_source(fs,
                                                  gvir_sandbox_config_mount_file_get_source(mfile));
            gvir_config_domain_filesys_set_target(fs,
                                                  gvir_sandbox_config_mount_get_target(mconfig));

            gvir_config_domain_add_device(domain,
                                          GVIR_CONFIG_DOMAIN_DEVICE(fs));
            g_object_unref(fs);
        } else if (GVIR_SANDBOX_IS_CONFIG_MOUNT_GUEST_BIND(mconfig)) {
            GVirSandboxConfigMountFile *mfile = GVIR_SANDBOX_CONFIG_MOUNT_FILE(mconfig);

            fs = gvir_config_domain_filesys_new();
            gvir_config_domain_filesys_set_type(fs, GVIR_CONFIG_DOMAIN_FILESYS_BIND);
            gvir_config_domain_filesys_set_access_type(fs, GVIR_CONFIG_DOMAIN_FILESYS_ACCESS_PASSTHROUGH);
            gvir_config_domain_filesys_set_source(fs,
                                                  gvir_sandbox_config_mount_file_get_source(mfile));
            gvir_config_domain_filesys_set_target(fs,
                                                  gvir_sandbox_config_mount_get_target(mconfig));

            gvir_config_domain_add_device(domain,
                                          GVIR_CONFIG_DOMAIN_DEVICE(fs));
            g_object_unref(fs);
        } else if (GVIR_SANDBOX_IS_CONFIG_MOUNT_RAM(mconfig)) {
            GVirSandboxConfigMountRam *mram = GVIR_SANDBOX_CONFIG_MOUNT_RAM(mconfig);

            fs = gvir_config_domain_filesys_new();
            gvir_config_domain_filesys_set_type(fs, GVIR_CONFIG_DOMAIN_FILESYS_RAM);
            gvir_config_domain_filesys_set_access_type(fs, GVIR_CONFIG_DOMAIN_FILESYS_ACCESS_PASSTHROUGH);
            gvir_config_domain_filesys_set_ram_usage(fs,
                                                     gvir_sandbox_config_mount_ram_get_usage(mram));
            gvir_config_domain_filesys_set_target(fs,
                                                  gvir_sandbox_config_mount_get_target(mconfig));

            gvir_config_domain_add_device(domain,
                                          GVIR_CONFIG_DOMAIN_DEVICE(fs));
            g_object_unref(fs);
        }

        tmp = tmp->next;
    }
    g_list_foreach(mounts, (GFunc)g_object_unref, NULL);
    g_list_free(mounts);


    tmp = networks = gvir_sandbox_config_get_networks(config);
    while (tmp) {
        const gchar *source, *mac;
        GVirSandboxConfigNetwork *network = GVIR_SANDBOX_CONFIG_NETWORK(tmp->data);

        iface = gvir_config_domain_interface_network_new();
        source = gvir_sandbox_config_network_get_source(network);
        if (source)
            gvir_config_domain_interface_network_set_source(iface, source);
        else
            gvir_config_domain_interface_network_set_source(iface, "default");

        mac = gvir_sandbox_config_network_get_mac(network);
        if (mac)
            gvir_config_domain_interface_set_mac(GVIR_CONFIG_DOMAIN_INTERFACE(iface),
                                                 mac);

        gvir_config_domain_add_device(domain,
                                      GVIR_CONFIG_DOMAIN_DEVICE(iface));
        g_object_unref(iface);

        tmp = tmp->next;
    }
    g_list_foreach(networks, (GFunc)g_object_unref, NULL);
    g_list_free(networks);


    /* The first console is for debug messages from stdout/err of the guest init/kernel */
    src = gvir_config_domain_chardev_source_pty_new();
    con = gvir_config_domain_console_new();
    gvir_config_domain_chardev_set_source(GVIR_CONFIG_DOMAIN_CHARDEV(con),
                                          GVIR_CONFIG_DOMAIN_CHARDEV_SOURCE(src));
    gvir_config_domain_add_device(domain,
                                  GVIR_CONFIG_DOMAIN_DEVICE(con));
    g_object_unref(con);


    /* Optional second console is for a interactive shell (useful for
     * troubleshooting stuff */
    if (gvir_sandbox_config_get_shell(config)) {
        src = gvir_config_domain_chardev_source_pty_new();
        con = gvir_config_domain_console_new();
        gvir_config_domain_chardev_set_source(GVIR_CONFIG_DOMAIN_CHARDEV(con),
                                              GVIR_CONFIG_DOMAIN_CHARDEV_SOURCE(src));
        gvir_config_domain_add_device(domain,
                                      GVIR_CONFIG_DOMAIN_DEVICE(con));
        g_object_unref(con);
    }

    if (GVIR_SANDBOX_IS_CONFIG_INTERACTIVE(config)) {
        /* The third console is for stdio of the sandboxed app */
        src = gvir_config_domain_chardev_source_pty_new();
        con = gvir_config_domain_console_new();
        gvir_config_domain_chardev_set_source(GVIR_CONFIG_DOMAIN_CHARDEV(con),
                                              GVIR_CONFIG_DOMAIN_CHARDEV_SOURCE(src));
        gvir_config_domain_add_device(domain,
                                      GVIR_CONFIG_DOMAIN_DEVICE(con));
        g_object_unref(con);
    }

    ret = TRUE;
cleanup:
    g_free(configdir);
    return ret;
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
 * Create a new pplication sandbox builder for containers
 *
 * Returns: (transfer full): a new sandbox builder object
 */
GVirSandboxBuilderContainer *gvir_sandbox_builder_container_new(GVirConnection *connection)
{
    return GVIR_SANDBOX_BUILDER_CONTAINER(g_object_new(GVIR_SANDBOX_TYPE_BUILDER_CONTAINER,
                                                       "connection", connection,
                                                       NULL));
}
