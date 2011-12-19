/*
 * libvirt-sandbox-builder-machine.c: libvirt sandbox configuration
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
#include <sys/utsname.h>

#include "libvirt-sandbox/libvirt-sandbox.h"

/**
 * SECTION: libvirt-sandbox-builder-machine
 * @short_description: Sandbox virtual machine construction
 * @include: libvirt-sandbox/libvirt-sandbox.h
 * @see_also: #GVirSandboxBuilder, #GVirSandboxBuilderMachine
 *
 * Provides an object for creating sandboxes using machine virtualization
 *
 * The GVirSandboxBuilderContainer object provides a way to builder sandboxes
 * using full machine virtualization technologies such as KVM.
 */

#define GVIR_SANDBOX_BUILDER_MACHINE_GET_PRIVATE(obj)                         \
        (G_TYPE_INSTANCE_GET_PRIVATE((obj), GVIR_SANDBOX_TYPE_BUILDER_MACHINE, GVirSandboxBuilderMachinePrivate))

struct _GVirSandboxBuilderMachinePrivate
{
    gboolean unused;
};

G_DEFINE_TYPE(GVirSandboxBuilderMachine, gvir_sandbox_builder_machine, GVIR_SANDBOX_TYPE_BUILDER);


enum {
    PROP_0,
};

enum {
    LAST_SIGNAL
};

//static gint signals[LAST_SIGNAL];

#define GVIR_SANDBOX_BUILDER_MACHINE_ERROR gvir_sandbox_builder_machine_error_quark()

static GQuark
gvir_sandbox_builder_machine_error_quark(void)
{
    return g_quark_from_static_string("gvir-sandbox-builder-machine");
}


static void gvir_sandbox_builder_machine_get_property(GObject *object,
                                                      guint prop_id,
                                                      GValue *value G_GNUC_UNUSED,
                                                      GParamSpec *pspec)
{
#if 0
    GVirSandboxBuilderMachine *builder = GVIR_SANDBOX_BUILDER_MACHINE(object);
    GVirSandboxBuilderMachinePrivate *priv = builder->priv;
#endif

    switch (prop_id) {

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_builder_machine_set_property(GObject *object,
                                                      guint prop_id,
                                                      const GValue *value G_GNUC_UNUSED,
                                                      GParamSpec *pspec)
{
#if 0
    GVirSandboxBuilderMachine *builder = GVIR_SANDBOX_BUILDER_MACHINE(object);
    GVirSandboxBuilderMachinePrivate *priv = builder->priv;
#endif

    switch (prop_id) {

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_builder_machine_finalize(GObject *object)
{
#if 0
    GVirSandboxBuilderMachine *builder = GVIR_SANDBOX_BUILDER_MACHINE(object);
    GVirSandboxBuilderMachinePrivate *priv = builder->priv;
#endif

    G_OBJECT_CLASS(gvir_sandbox_builder_machine_parent_class)->finalize(object);
}


static gchar *gvir_sandbox_builder_machine_mkinitrd(const gchar *kver,
                                                    GError **error)
{
    GVirSandboxConfigInitrd *cfg = gvir_sandbox_config_initrd_new();
    GVirSandboxBuilderInitrd *builder = gvir_sandbox_builder_initrd_new();
    gchar *targetfile = g_strdup_printf("/tmp/libvirt-sandbox-initrd-XXXXXX");
    int fd = -1;

    gvir_sandbox_config_initrd_set_kver(cfg, kver);
    gvir_sandbox_config_initrd_set_init(cfg, LIBEXECDIR "/libvirt-sandbox-init-qemu");

    gvir_sandbox_config_initrd_add_module(cfg, "fscache.ko");
    gvir_sandbox_config_initrd_add_module(cfg, "9pnet.ko");
    gvir_sandbox_config_initrd_add_module(cfg, "9p.ko");
    gvir_sandbox_config_initrd_add_module(cfg, "9pnet_virtio.ko");
#if 0
    gvir_sandbox_config_initrd_add_module(cfg, "virtio_net.ko");
    gvir_sandbox_config_initrd_add_module(cfg, "virtio_balloon.ko");
#endif

    if ((fd = mkstemp(targetfile)) < 0) {
        g_set_error(error, GVIR_SANDBOX_BUILDER_MACHINE_ERROR, 0,
                    "Cannot create initrd %s", targetfile);
        goto cleanup;
    }

    if (!gvir_sandbox_builder_initrd_construct(builder, cfg, targetfile, error))
        goto cleanup;

cleanup:
    if (fd != -1)
        close(fd);
    if (*error) {
        g_free(targetfile);
        targetfile = NULL;
    }
    g_object_unref(cfg);
    g_object_unref(builder);
    return targetfile;
}


static gchar *gvir_sandbox_builder_machine_cmdline(GVirSandboxConfig *config)
{
    GString *str = g_string_new("");
    gchar **argv, **tmp;
    gchar *cmdrawstr;
    gchar *cmdb64str;
    gsize len = 0, offset = 0;
    gchar *ret;
    gchar *lenstr;
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

    if (GVIR_SANDBOX_IS_CONFIG_GRAPHICAL(config)) {
        GVirSandboxConfigGraphical *gconfig = GVIR_SANDBOX_CONFIG_GRAPHICAL(config);
        g_string_append(str, " sandbox-xorg");
        g_string_append_printf(str, " sandbox-vm=%s",
                               gvir_sandbox_config_graphical_get_window_manager(gconfig));
    }

    /* Now kernel args */
    g_string_append(str, " console=ttyS0");
    if (getenv("LIBVIRT_SANDBOX_DEBUG"))
        g_string_append(str, " debug loglevel=10 earlyprintk=ttyS0");
    else
        g_string_append(str, " quiet loglevel=0");

    /* These make boot a little bit faster */
    g_string_append(str, " edd=off");
    g_string_append(str, " printk.time=1");
    g_string_append(str, " noreplace-smp");
    g_string_append(str, " cgroup_disable=memory");
    g_string_append(str, " pci=noearly");

    ret = str->str;
    g_string_free(str, FALSE);
    return ret;
}


static gboolean gvir_sandbox_builder_machine_delete(GVirSandboxCleaner *cleaner G_GNUC_UNUSED,
                                                    GError **error G_GNUC_UNUSED,
                                                    gpointer opaque)
{
    gchar *path = opaque;
    unlink(path);
    return TRUE;
}

static gboolean gvir_sandbox_builder_machine_construct_basic(GVirSandboxBuilder *builder,
                                                             GVirSandboxConfig *config,
                                                             GVirSandboxCleaner *cleaner,
                                                             GVirConfigDomain *domain,
                                                             GError **error)
{
    if (!GVIR_SANDBOX_BUILDER_CLASS(gvir_sandbox_builder_machine_parent_class)->
        construct_basic(builder, config, cleaner, domain, error))
        return FALSE;

    gvir_config_domain_set_virt_type(domain,
                                     GVIR_CONFIG_DOMAIN_VIRT_KVM);

    return TRUE;
}


static gboolean gvir_sandbox_builder_machine_construct_os(GVirSandboxBuilder *builder,
                                                          GVirSandboxConfig *config,
                                                          GVirSandboxCleaner *cleaner,
                                                          GVirConfigDomain *domain,
                                                          GError **error)
{
    gchar *kernel = NULL;
    gchar *initrd = NULL;
    gchar *cmdline = NULL;
    struct utsname uts;
    GVirConfigDomainOs *os;

    if (!GVIR_SANDBOX_BUILDER_CLASS(gvir_sandbox_builder_machine_parent_class)->
        construct_os(builder, config, cleaner, domain, error))
        return FALSE;

    uname(&uts);

    if (!(initrd = gvir_sandbox_builder_machine_mkinitrd(uts.release, error)))
        return FALSE;

    kernel = g_strdup_printf("/boot/vmlinuz-%s", uts.release);
    cmdline = gvir_sandbox_builder_machine_cmdline(config);

    gvir_sandbox_cleaner_add_action_post_start(cleaner,
                                               gvir_sandbox_builder_machine_delete,
                                               initrd,
                                               g_free);

    os = gvir_config_domain_os_new();
    gvir_config_domain_os_set_os_type(os,
                                      GVIR_CONFIG_DOMAIN_OS_TYPE_HVM);
    gvir_config_domain_os_set_arch(os,
                                   gvir_sandbox_config_get_arch(config));
    gvir_config_domain_os_set_kernel(os, kernel);
    gvir_config_domain_os_set_ramdisk(os, initrd);
    gvir_config_domain_os_set_cmdline(os, cmdline);

    gvir_config_domain_set_os(domain, os);

    g_free(kernel);
    g_free(cmdline);
    /* initrd is free'd by the cleaner */

    return TRUE;
}


static gboolean gvir_sandbox_builder_machine_construct_features(GVirSandboxBuilder *builder,
                                                                GVirSandboxConfig *config,
                                                                GVirSandboxCleaner *cleaner,
                                                                GVirConfigDomain *domain,
                                                                GError **error)
{
    gchar **features;

    if (!GVIR_SANDBOX_BUILDER_CLASS(gvir_sandbox_builder_machine_parent_class)->
        construct_features(builder, config, cleaner, domain, error))
        return FALSE;

    features = g_new0(gchar *, 1);
    features[0] = g_strdup("acpi");
    gvir_config_domain_set_features(domain, features);
    g_strfreev(features);
    return TRUE;
}

static gboolean gvir_sandbox_builder_machine_construct_devices(GVirSandboxBuilder *builder,
                                                               GVirSandboxConfig *config,
                                                               GVirSandboxCleaner *cleaner,
                                                               GVirConfigDomain *domain,
                                                               GError **error)
{
    GVirConfigDomainFilesys *fs;
    GVirConfigDomainMemballoon *ball;
    GVirConfigDomainGraphicsSpice *graph;
    GList *tmp = NULL, *mounts = NULL;

    if (!GVIR_SANDBOX_BUILDER_CLASS(gvir_sandbox_builder_machine_parent_class)->
        construct_devices(builder, config, cleaner, domain, error))
        return FALSE;

    fs = gvir_config_domain_filesys_new();
    gvir_config_domain_filesys_set_type(fs, GVIR_CONFIG_DOMAIN_FILESYS_MOUNT);
    gvir_config_domain_filesys_set_access_type(fs, GVIR_CONFIG_DOMAIN_FILESYS_ACCESS_PASSTHROUGH);
    gvir_config_domain_filesys_set_source(fs,
                                          gvir_sandbox_config_get_root(config));
    gvir_config_domain_filesys_set_target(fs, "sandbox:root");
    gvir_config_domain_filesys_set_readonly(fs, TRUE);

    gvir_config_domain_add_device(domain,
                                  GVIR_CONFIG_DOMAIN_DEVICE(fs));
    g_object_unref(fs);

    tmp = mounts = gvir_sandbox_config_get_host_mounts(config);
    while (tmp) {
        GVirSandboxConfigMount *mconfig = tmp->data;
        const gchar *home = getenv("HOME");
        const gchar *tgtsym;
        const gchar *target = gvir_sandbox_config_mount_get_target(mconfig);
        if (home &&
            (g_str_equal(target, home)))
            tgtsym = "sandbox:home";
        else if (g_str_equal(target, "/tmp"))
            tgtsym = "sandbox:tmp";
        else {
            g_set_error(error, GVIR_SANDBOX_BUILDER_MACHINE_ERROR, 0,
                        "Cannot export mount to %s", target);
            return FALSE;
        }

        fs = gvir_config_domain_filesys_new();
        gvir_config_domain_filesys_set_type(fs, GVIR_CONFIG_DOMAIN_FILESYS_MOUNT);
        gvir_config_domain_filesys_set_access_type(fs, GVIR_CONFIG_DOMAIN_FILESYS_ACCESS_PASSTHROUGH);
        gvir_config_domain_filesys_set_source(fs,
                                              gvir_sandbox_config_mount_get_root(mconfig));
        gvir_config_domain_filesys_set_target(fs, tgtsym);

        gvir_config_domain_add_device(domain,
                                      GVIR_CONFIG_DOMAIN_DEVICE(fs));
        g_object_unref(fs);

        tmp = tmp->next;
    }


    ball = gvir_config_domain_memballoon_new();
    gvir_config_domain_memballoon_set_model(ball,
                                            GVIR_CONFIG_DOMAIN_MEMBALLOON_MODEL_NONE);
    gvir_config_domain_add_device(domain,
                                  GVIR_CONFIG_DOMAIN_DEVICE(ball));
    g_object_unref(ball);


    if (GVIR_SANDBOX_IS_CONFIG_GRAPHICAL(config)) {
        graph = gvir_config_domain_graphics_spice_new();
        gvir_config_domain_graphics_spice_set_port(graph, -1);
        gvir_config_domain_add_device(domain,
                                      GVIR_CONFIG_DOMAIN_DEVICE(graph));
        g_object_unref(graph);
    }

    return TRUE;
}


static void gvir_sandbox_builder_machine_class_init(GVirSandboxBuilderMachineClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    GVirSandboxBuilderClass *builder_class = GVIR_SANDBOX_BUILDER_CLASS(klass);

    object_class->finalize = gvir_sandbox_builder_machine_finalize;
    object_class->get_property = gvir_sandbox_builder_machine_get_property;
    object_class->set_property = gvir_sandbox_builder_machine_set_property;

    builder_class->construct_basic = gvir_sandbox_builder_machine_construct_basic;
    builder_class->construct_os = gvir_sandbox_builder_machine_construct_os;
    builder_class->construct_features = gvir_sandbox_builder_machine_construct_features;
    builder_class->construct_devices = gvir_sandbox_builder_machine_construct_devices;

    g_type_class_add_private(klass, sizeof(GVirSandboxBuilderMachinePrivate));
}


static void gvir_sandbox_builder_machine_init(GVirSandboxBuilderMachine *builder)
{
    builder->priv = GVIR_SANDBOX_BUILDER_MACHINE_GET_PRIVATE(builder);
}


/**
 * gvir_sandbox_builder_machine_new:
 * @connection: (transfer none): the connection
 *
 * Create a new graphical application sandbox builderuration
 *
 * Returns: (transfer full): a new graphical sandbox builder object
 */
GVirSandboxBuilderMachine *gvir_sandbox_builder_machine_new(GVirConnection *connection)
{
    return GVIR_SANDBOX_BUILDER_MACHINE(g_object_new(GVIR_SANDBOX_TYPE_BUILDER_MACHINE,
                                                     "connection", connection,
                                                     NULL));
}
