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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Daniel P. Berrange <berrange@redhat.com>
 */

#include <config.h>
#include <string.h>
#include <errno.h>
#include <sys/utsname.h>

#include <glib/gi18n.h>

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

#if 0
#define GVIR_SANDBOX_BUILDER_MACHINE_ERROR gvir_sandbox_builder_machine_error_quark()

static GQuark
gvir_sandbox_builder_machine_error_quark(void)
{
    return g_quark_from_static_string("gvir-sandbox-builder-machine");
}
#endif

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


static gchar *gvir_sandbox_builder_machine_get_kernrelease(GVirSandboxConfig *config)
{
    struct utsname uts;
    const gchar *kver = gvir_sandbox_config_get_kernrelease(config);
    if (kver)
        return g_strdup(kver);

    uname(&uts);
    return g_strdup(uts.release);
}


static gchar *gvir_sandbox_builder_machine_get_kernpath(GVirSandboxConfig *config)
{
    const gchar *kernpath = gvir_sandbox_config_get_kernpath(config);
    gchar *kver;
    gchar *ret;
    if (kernpath)
        return g_strdup(kernpath);

    kver = gvir_sandbox_builder_machine_get_kernrelease(config);
    ret = g_strdup_printf("/boot/vmlinuz-%s", kver);
    g_free(kver);
    return ret;
}


static gchar *gvir_sandbox_builder_machine_mkinitrd(GVirSandboxConfig *config,
                                                    const char *statedir,
                                                    GError **error)
{
    GVirSandboxConfigInitrd *initrd = gvir_sandbox_config_initrd_new();
    GVirSandboxBuilderInitrd *builder = gvir_sandbox_builder_initrd_new();
    gchar *targetfile = g_strdup_printf("%s/initrd.img", statedir);
    int fd = -1;
    gchar *kver = gvir_sandbox_builder_machine_get_kernrelease(config);
    const gchar *kmodpath = gvir_sandbox_config_get_kmodpath(config);
    if (!kmodpath)
        kmodpath = "/lib/modules";

    gvir_sandbox_config_initrd_set_kver(initrd, kver);
    gchar *kmoddir = g_strdup_printf("%s/%s/kernel",
                                     kmodpath, kver);
    gvir_sandbox_config_initrd_set_kmoddir(initrd, kmoddir);

    gvir_sandbox_config_initrd_set_init(initrd, LIBEXECDIR "/libvirt-sandbox-init-qemu");

    gvir_sandbox_config_initrd_add_module(initrd, "fscache.ko");
    gvir_sandbox_config_initrd_add_module(initrd, "9pnet.ko");
    gvir_sandbox_config_initrd_add_module(initrd, "9p.ko");
    gvir_sandbox_config_initrd_add_module(initrd, "9pnet_virtio.ko");
    if (gvir_sandbox_config_has_networks(config))
        gvir_sandbox_config_initrd_add_module(initrd, "virtio_net.ko");
    if (gvir_sandbox_config_has_mounts_with_type(config,
                                                 GVIR_SANDBOX_TYPE_CONFIG_MOUNT_HOST_IMAGE))
        gvir_sandbox_config_initrd_add_module(initrd, "virtio_blk.ko");
#if 0
    gvir_sandbox_config_initrd_add_module(initrd, "virtio_balloon.ko");
#endif

    if (!gvir_sandbox_builder_initrd_construct(builder, initrd, targetfile, error))
        goto cleanup;

cleanup:
    if (fd != -1)
        close(fd);
    if (*error) {
        g_free(targetfile);
        targetfile = NULL;
    }
    g_free(kmoddir);
    g_free(kver);
    g_object_unref(initrd);
    g_object_unref(builder);
    return targetfile;
}


static gchar *gvir_sandbox_builder_machine_cmdline(GVirSandboxConfig *config G_GNUC_UNUSED)
{
    GString *str = g_string_new("");
    gchar *ret;
    gchar *tmp;

    /* Now kernel args */
    g_string_append(str, " console=ttyS0");
    if (getenv("LIBVIRT_SANDBOX_DEBUG") &&
        g_str_equal(getenv("LIBVIRT_SANDBOX_DEBUG"), "2"))
        g_string_append(str, " debug loglevel=10 earlyprintk=ttyS0");
    else
        g_string_append(str, " quiet loglevel=0");

    if ((tmp = getenv("LIBVIRT_SANDBOX_STRACE"))) {
        g_string_append(str, " strace");
        if (!g_str_equal(tmp, "1")) {
            g_string_append(str, "=");
            g_string_append(str, tmp);
        }
    }

    /* These make boot a little bit faster */
    g_string_append(str, " edd=off");
    g_string_append(str, " printk.time=1");
    g_string_append(str, " noreplace-smp");
    g_string_append(str, " cgroup_disable=memory");
    g_string_append(str, " pci=noearly");
    /* Running QEMU inside KVM makes APIC unreliable */
    g_string_append(str, " noapic");
    /* Reboot immediately on panic */
    g_string_append(str, " panic=-1");
    /* Disable SELinux, since the program we're about to run
     * is likely to be unconfined anyway */
    /* XXX re-visit this to see if we can use it with
     * service based sandboxes */
    g_string_append(str, " selinux=0");

    ret = str->str;
    g_string_free(str, FALSE);
    return ret;
}


static gboolean gvir_sandbox_builder_machine_write_mount_cfg(GVirSandboxConfig *config,
                                                             const gchar *statedir,
                                                             GError **error)
{
    gchar *mntfile = g_strdup_printf("%s/config/mounts.cfg", statedir);
    GFile *file = g_file_new_for_path(mntfile);
    GFileOutputStream *fos = g_file_replace(file,
                                            NULL,
                                            FALSE,
                                            G_FILE_CREATE_REPLACE_DESTINATION,
                                            NULL,
                                            error);
    gboolean ret = FALSE;
    GList *mounts = gvir_sandbox_config_get_mounts(config);
    GList *tmp = NULL;
    size_t nHostBind = 0;
    size_t nHostImage = 0;

    if (!fos)
        goto cleanup;

    tmp = mounts;
    while (tmp) {
        GVirSandboxConfigMount *mconfig = GVIR_SANDBOX_CONFIG_MOUNT(tmp->data);
        const gchar *fstype;
        gchar *source;
        gchar *options;
        const gchar *target;
        gchar *line;

        if (GVIR_SANDBOX_IS_CONFIG_MOUNT_HOST_BIND(mconfig)) {
            source = g_strdup_printf("sandbox:mount%zu", nHostBind++);
            fstype = "9p";
            options = g_strdup("trans=virtio");
        } else if (GVIR_SANDBOX_IS_CONFIG_MOUNT_HOST_IMAGE(mconfig)) {
            source = g_strdup_printf("vd%c", (char)('a' + nHostImage++));
            fstype = "ext3";
            options = g_strdup("");
        } else if (GVIR_SANDBOX_IS_CONFIG_MOUNT_GUEST_BIND(mconfig)) {
            GVirSandboxConfigMountFile *mfile = GVIR_SANDBOX_CONFIG_MOUNT_FILE(mconfig);
            source = g_strdup(gvir_sandbox_config_mount_file_get_source(mfile));
            fstype = "";
            options = g_strdup("");
        } else if (GVIR_SANDBOX_IS_CONFIG_MOUNT_RAM(mconfig)) {
            GVirSandboxConfigMountRam *mram = GVIR_SANDBOX_CONFIG_MOUNT_RAM(mconfig);
            source = g_strdup("tmpfs");
            fstype = "tmpfs";
            options = g_strdup_printf("size=%" G_GUINT64_FORMAT "k",
                                      gvir_sandbox_config_mount_ram_get_usage(mram)/1024);
        } else {
            g_assert_not_reached();
        }
        target = gvir_sandbox_config_mount_get_target(mconfig);

        line = g_strdup_printf("%s\t%s\t%s\t%s\n",
                               source, target, fstype, options);
        g_free(source);
        g_free(options);

        if (!g_output_stream_write_all(G_OUTPUT_STREAM(fos),
                                       line, strlen(line),
                                       NULL, NULL, error)) {
            g_free(line);
            goto cleanup;
        }
        g_free(line);

        tmp = tmp->next;
    }

    if (!g_output_stream_close(G_OUTPUT_STREAM(fos), NULL, error))
        goto cleanup;

#if 0
    gvir_sandbox_cleaner_add_rmfile_post_stop(cleaner,
                                              mntfile);
#endif

    ret = TRUE;
cleanup:
    g_list_foreach(mounts, (GFunc)g_object_unref, NULL);
    g_list_free(mounts);
    if (fos)
        g_object_unref(fos);
    if (!ret)
        g_file_delete(file, NULL, NULL);
    g_object_unref(file);
    g_free(mntfile);
    return ret;
}


static gboolean gvir_sandbox_builder_machine_construct_domain(GVirSandboxBuilder *builder,
                                                              GVirSandboxConfig *config,
                                                              const gchar *statedir,
                                                              GVirConfigDomain *domain,
                                                              GError **error)
{
    if (!gvir_sandbox_builder_machine_write_mount_cfg(config,
                                                      statedir,
                                                      error))
        return FALSE;

    if (!GVIR_SANDBOX_BUILDER_CLASS(gvir_sandbox_builder_machine_parent_class)->
        construct_domain(builder, config, statedir, domain, error))
        return FALSE;

    return TRUE;
}

static gboolean gvir_sandbox_builder_machine_construct_basic(GVirSandboxBuilder *builder,
                                                             GVirSandboxConfig *config,
                                                             const gchar *statedir,
                                                             GVirConfigDomain *domain,
                                                             GError **error)
{
    if (!GVIR_SANDBOX_BUILDER_CLASS(gvir_sandbox_builder_machine_parent_class)->
        construct_basic(builder, config, statedir, domain, error))
        return FALSE;

    if (access("/dev/kvm", W_OK) == 0)
        gvir_config_domain_set_virt_type(domain,
                                         GVIR_CONFIG_DOMAIN_VIRT_KVM);
    else
        gvir_config_domain_set_virt_type(domain,
                                         GVIR_CONFIG_DOMAIN_VIRT_QEMU);

    gvir_config_domain_set_lifecycle(domain,
                                     GVIR_CONFIG_DOMAIN_LIFECYCLE_ON_POWEROFF,
                                     GVIR_CONFIG_DOMAIN_LIFECYCLE_DESTROY);
    gvir_config_domain_set_lifecycle(domain,
                                     GVIR_CONFIG_DOMAIN_LIFECYCLE_ON_REBOOT,
                                     GVIR_CONFIG_DOMAIN_LIFECYCLE_DESTROY);
    gvir_config_domain_set_lifecycle(domain,
                                     GVIR_CONFIG_DOMAIN_LIFECYCLE_ON_CRASH,
                                     GVIR_CONFIG_DOMAIN_LIFECYCLE_DESTROY);

    return TRUE;
}


static gboolean gvir_sandbox_builder_machine_construct_os(GVirSandboxBuilder *builder,
                                                          GVirSandboxConfig *config,
                                                          const gchar *statedir,
                                                          GVirConfigDomain *domain,
                                                          GError **error)
{
    gchar *kernel = NULL;
    gchar *initrd = NULL;
    gchar *cmdline = NULL;
    GVirConfigDomainOs *os;

    if (!GVIR_SANDBOX_BUILDER_CLASS(gvir_sandbox_builder_machine_parent_class)->
        construct_os(builder, config, statedir, domain, error))
        return FALSE;

    if (!(initrd = gvir_sandbox_builder_machine_mkinitrd(config,
                                                         statedir,
                                                         error)))
        return FALSE;

    kernel = gvir_sandbox_builder_machine_get_kernpath(config);
    cmdline = gvir_sandbox_builder_machine_cmdline(config);

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
    g_free(initrd);

    return TRUE;
}


static gboolean gvir_sandbox_builder_machine_construct_features(GVirSandboxBuilder *builder,
                                                                GVirSandboxConfig *config,
                                                                const gchar *statedir,
                                                                GVirConfigDomain *domain,
                                                                GError **error)
{
    gchar **features;

    if (!GVIR_SANDBOX_BUILDER_CLASS(gvir_sandbox_builder_machine_parent_class)->
        construct_features(builder, config, statedir, domain, error))
        return FALSE;

    features = g_new0(gchar *, 2);
    features[0] = g_strdup("acpi");
    gvir_config_domain_set_features(domain, features);
    g_strfreev(features);
    return TRUE;
}

static gboolean gvir_sandbox_builder_machine_construct_devices(GVirSandboxBuilder *builder,
                                                               GVirSandboxConfig *config,
                                                               const gchar *statedir,
                                                               GVirConfigDomain *domain,
                                                               GError **error)
{
    GVirConfigDomainFilesys *fs;
    GVirConfigDomainDisk *disk;
    GVirConfigDomainInterface *iface;
    GVirConfigDomainMemballoon *ball;
    GVirConfigDomainConsole *con;
    GVirConfigDomainSerial *ser;
    GVirConfigDomainChardevSourcePty *src;
    GList *tmp = NULL, *mounts = NULL, *networks = NULL;
    size_t nHostBind = 0;
    size_t nHostImage = 0;
    gchar *configdir = g_strdup_printf("%s/config", statedir);
    gboolean ret = FALSE;

    if (!GVIR_SANDBOX_BUILDER_CLASS(gvir_sandbox_builder_machine_parent_class)->
        construct_devices(builder, config, statedir, domain, error))
        goto cleanup;

    fs = gvir_config_domain_filesys_new();
    gvir_config_domain_filesys_set_type(fs, GVIR_CONFIG_DOMAIN_FILESYS_MOUNT);
    gvir_config_domain_filesys_set_access_type(fs, GVIR_CONFIG_DOMAIN_FILESYS_ACCESS_SQUASH);
    gvir_config_domain_filesys_set_source(fs,
                                          gvir_sandbox_config_get_root(config));
    gvir_config_domain_filesys_set_target(fs, "sandbox:root");
    gvir_config_domain_filesys_set_readonly(fs, TRUE);

    gvir_config_domain_add_device(domain,
                                  GVIR_CONFIG_DOMAIN_DEVICE(fs));
    g_object_unref(fs);


    fs = gvir_config_domain_filesys_new();
    gvir_config_domain_filesys_set_type(fs, GVIR_CONFIG_DOMAIN_FILESYS_MOUNT);
    gvir_config_domain_filesys_set_access_type(fs, GVIR_CONFIG_DOMAIN_FILESYS_ACCESS_SQUASH);
    gvir_config_domain_filesys_set_source(fs, configdir);
    gvir_config_domain_filesys_set_target(fs, "sandbox:config");
    gvir_config_domain_filesys_set_readonly(fs, TRUE);

    gvir_config_domain_add_device(domain,
                                  GVIR_CONFIG_DOMAIN_DEVICE(fs));
    g_object_unref(fs);


    tmp = mounts = gvir_sandbox_config_get_mounts(config);
    while (tmp) {
        GVirSandboxConfigMount *mconfig = GVIR_SANDBOX_CONFIG_MOUNT(tmp->data);

        if (GVIR_SANDBOX_IS_CONFIG_MOUNT_HOST_BIND(mconfig)) {
            GVirSandboxConfigMountFile *mfile = GVIR_SANDBOX_CONFIG_MOUNT_FILE(mconfig);

            gchar *target = g_strdup_printf("sandbox:mount%zu", nHostBind++);

            fs = gvir_config_domain_filesys_new();
            gvir_config_domain_filesys_set_type(fs, GVIR_CONFIG_DOMAIN_FILESYS_MOUNT);
            gvir_config_domain_filesys_set_access_type(fs, GVIR_CONFIG_DOMAIN_FILESYS_ACCESS_MAPPED);
            gvir_config_domain_filesys_set_source(fs,
                                                  gvir_sandbox_config_mount_file_get_source(mfile));
            gvir_config_domain_filesys_set_target(fs, target);

            gvir_config_domain_add_device(domain,
                                          GVIR_CONFIG_DOMAIN_DEVICE(fs));
            g_object_unref(fs);
            g_free(target);

        } else if (GVIR_SANDBOX_IS_CONFIG_MOUNT_HOST_IMAGE(mconfig)) {
            GVirSandboxConfigMountFile *mfile = GVIR_SANDBOX_CONFIG_MOUNT_FILE(mconfig);
            gchar *target = g_strdup_printf("vd%c", (char)('a' + nHostImage++));

            disk = gvir_config_domain_disk_new();
            gvir_config_domain_disk_set_type(disk, GVIR_CONFIG_DOMAIN_DISK_FILE);
            gvir_config_domain_disk_set_source(disk,
                                               gvir_sandbox_config_mount_file_get_source(mfile));
            gvir_config_domain_disk_set_target_dev(disk, target);

            gvir_config_domain_add_device(domain,
                                          GVIR_CONFIG_DOMAIN_DEVICE(disk));
            g_object_unref(disk);
            g_free(target);
        }
        tmp = tmp->next;
    }
    g_list_foreach(mounts, (GFunc)g_object_unref, NULL);
    g_list_free(mounts);

    tmp = networks = gvir_sandbox_config_get_networks(config);
    while (tmp) {
        const gchar *source, *mac;
        GVirSandboxConfigNetwork *network = GVIR_SANDBOX_CONFIG_NETWORK(tmp->data);

        source = gvir_sandbox_config_network_get_source(network);
        if (source) {
            iface = GVIR_CONFIG_DOMAIN_INTERFACE(gvir_config_domain_interface_network_new());
            gvir_config_domain_interface_network_set_source(
                GVIR_CONFIG_DOMAIN_INTERFACE_NETWORK(iface), source);
        } else {
            iface = GVIR_CONFIG_DOMAIN_INTERFACE(gvir_config_domain_interface_user_new());
        }

        mac = gvir_sandbox_config_network_get_mac(network);
        if (mac)
            gvir_config_domain_interface_set_mac(iface, mac);

        gvir_config_domain_interface_set_model(iface,
                                               "virtio");

        gvir_config_domain_add_device(domain,
                                      GVIR_CONFIG_DOMAIN_DEVICE(iface));
        g_object_unref(iface);

        tmp = tmp->next;
    }
    g_list_foreach(networks, (GFunc)g_object_unref, NULL);
    g_list_free(networks);


    ball = gvir_config_domain_memballoon_new();
    gvir_config_domain_memballoon_set_model(ball,
                                            GVIR_CONFIG_DOMAIN_MEMBALLOON_MODEL_NONE);
    gvir_config_domain_add_device(domain,
                                  GVIR_CONFIG_DOMAIN_DEVICE(ball));
    g_object_unref(ball);


    /* The first serial is for stdio of the sandboxed app */
    src = gvir_config_domain_chardev_source_pty_new();
    con = gvir_config_domain_console_new();
    gvir_config_domain_console_set_target_type(GVIR_CONFIG_DOMAIN_CONSOLE(con),
                                               GVIR_CONFIG_DOMAIN_CONSOLE_TARGET_SERIAL);
    gvir_config_domain_chardev_set_source(GVIR_CONFIG_DOMAIN_CHARDEV(con),
                                          GVIR_CONFIG_DOMAIN_CHARDEV_SOURCE(src));
    gvir_config_domain_add_device(domain,
                                  GVIR_CONFIG_DOMAIN_DEVICE(con));
    g_object_unref(con);

    src = gvir_config_domain_chardev_source_pty_new();
    ser = gvir_config_domain_serial_new();
    gvir_config_domain_chardev_set_source(GVIR_CONFIG_DOMAIN_CHARDEV(ser),
                                          GVIR_CONFIG_DOMAIN_CHARDEV_SOURCE(src));
    gvir_config_domain_add_device(domain,
                                  GVIR_CONFIG_DOMAIN_DEVICE(ser));
    g_object_unref(ser);


    /* Optional second serial is for a interactive shell (useful for
     * troubleshooting stuff */
    if (gvir_sandbox_config_get_shell(config)) {
        src = gvir_config_domain_chardev_source_pty_new();
        ser = gvir_config_domain_serial_new();
        gvir_config_domain_chardev_set_source(GVIR_CONFIG_DOMAIN_CHARDEV(ser),
                                              GVIR_CONFIG_DOMAIN_CHARDEV_SOURCE(src));
        gvir_config_domain_add_device(domain,
                                      GVIR_CONFIG_DOMAIN_DEVICE(ser));
        g_object_unref(ser);
    }

    if (GVIR_SANDBOX_IS_CONFIG_INTERACTIVE(config)) {
        src = gvir_config_domain_chardev_source_pty_new();
        con = gvir_config_domain_console_new();
        gvir_config_domain_console_set_target_type(GVIR_CONFIG_DOMAIN_CONSOLE(con),
                                                   GVIR_CONFIG_DOMAIN_CONSOLE_TARGET_VIRTIO);
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



static gboolean gvir_sandbox_builder_machine_clean_post_start(GVirSandboxBuilder *builder,
                                                              GVirSandboxConfig *config,
                                                              const gchar *statedir,
                                                              GError **error)
{
    gchar *initrd = g_strdup_printf("%s/initrd.img", statedir);
    gboolean ret = TRUE;

    if (unlink(initrd) < 0 &&
        errno != ENOENT)
        ret = FALSE;

    g_free(initrd);
    return ret;
}


static gboolean gvir_sandbox_builder_machine_clean_post_stop(GVirSandboxBuilder *builder,
                                                             GVirSandboxConfig *config,
                                                             const gchar *statedir,
                                                             GError **error)
{
    gchar *mntfile = g_strdup_printf("%s/config/mounts.cfg", statedir);
    gboolean ret = TRUE;

    if (unlink(mntfile) < 0 &&
        errno != ENOENT)
        ret = FALSE;

    g_free(mntfile);
    return ret;
}


static void gvir_sandbox_builder_machine_class_init(GVirSandboxBuilderMachineClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    GVirSandboxBuilderClass *builder_class = GVIR_SANDBOX_BUILDER_CLASS(klass);

    object_class->finalize = gvir_sandbox_builder_machine_finalize;
    object_class->get_property = gvir_sandbox_builder_machine_get_property;
    object_class->set_property = gvir_sandbox_builder_machine_set_property;

    builder_class->construct_domain = gvir_sandbox_builder_machine_construct_domain;
    builder_class->construct_basic = gvir_sandbox_builder_machine_construct_basic;
    builder_class->construct_os = gvir_sandbox_builder_machine_construct_os;
    builder_class->construct_features = gvir_sandbox_builder_machine_construct_features;
    builder_class->construct_devices = gvir_sandbox_builder_machine_construct_devices;
    builder_class->clean_post_start = gvir_sandbox_builder_machine_clean_post_start;
    builder_class->clean_post_stop = gvir_sandbox_builder_machine_clean_post_stop;

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
 * Create a new application sandbox builder for virtual machines
 *
 * Returns: (transfer full): a new sandbox builder object
 */
GVirSandboxBuilderMachine *gvir_sandbox_builder_machine_new(GVirConnection *connection)
{
    return GVIR_SANDBOX_BUILDER_MACHINE(g_object_new(GVIR_SANDBOX_TYPE_BUILDER_MACHINE,
                                                     "connection", connection,
                                                     NULL));
}
