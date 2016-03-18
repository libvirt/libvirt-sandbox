/*
 * libvirt-sandbox-builder.c: libvirt sandbox builder
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

#include "libvirt-sandbox/libvirt-sandbox.h"
#include "libvirt-sandbox/libvirt-sandbox-builder-private.h"

/**
 * SECTION: libvirt-sandbox-builder
 * @short_description: Sandbox construction base class
 * @include: libvirt-sandbox/libvirt-sandbox.h
 * @see_also: #GVirSandboxBuilderContainer #GVirSandboxBuilderMachine
 *
 * Provides a base class for constructing sandboxes
 *
 * The GVirSandboxBuilder objects provides the basic framework for creating
 * #GVirDomain instances from #GVirSandboxConfig instances.
 */

#define GVIR_SANDBOX_BUILDER_GET_PRIVATE(obj)                           \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), GVIR_SANDBOX_TYPE_BUILDER, GVirSandboxBuilderPrivate))

struct _GVirSandboxBuilderPrivate
{
    GVirConnection *connection;
};

G_DEFINE_ABSTRACT_TYPE(GVirSandboxBuilder, gvir_sandbox_builder, G_TYPE_OBJECT);


enum {
    PROP_0,

    PROP_CONNECTION,
};

enum {
    LAST_SIGNAL
};

//static gint signals[LAST_SIGNAL];

#define GVIR_SANDBOX_BUILDER_ERROR gvir_sandbox_builder_error_quark()

static GQuark
gvir_sandbox_builder_error_quark(void)
{
    return g_quark_from_static_string("gvir-sandbox-builder");
}

static gboolean gvir_sandbox_builder_construct_domain(GVirSandboxBuilder *builder,
                                                      GVirSandboxConfig *config,
                                                      const gchar *statedir,
                                                      GVirConfigDomain *domain,
                                                      GError **error);
static gboolean gvir_sandbox_builder_construct_basic(GVirSandboxBuilder *builder,
                                                     GVirSandboxConfig *config,
                                                     const gchar *statedir,
                                                     GVirConfigDomain *domain,
                                                     GError **error);
static gboolean gvir_sandbox_builder_construct_os(GVirSandboxBuilder *builder,
                                                  GVirSandboxConfig *config,
                                                  const gchar *statedir,
                                                  GVirConfigDomain *domain,
                                                  GError **error);
static gboolean gvir_sandbox_builder_construct_features(GVirSandboxBuilder *builder,
                                                        GVirSandboxConfig *config,
                                                        const gchar *statedir,
                                                        GVirConfigDomain *domain,
                                                        GError **error);
static gboolean gvir_sandbox_builder_construct_devices(GVirSandboxBuilder *builder,
                                                       GVirSandboxConfig *config,
                                                       const gchar *statedir,
                                                       GVirConfigDomain *domain,
                                                       GError **error);
static gboolean gvir_sandbox_builder_construct_security(GVirSandboxBuilder *builder,
                                                        GVirSandboxConfig *config,
                                                        const gchar *statedir,
                                                        GVirConfigDomain *domain,
                                                        GError **error);
static gboolean gvir_sandbox_builder_clean_post_start_default(GVirSandboxBuilder *builder,
                                                              GVirSandboxConfig *config,
                                                              const gchar *statedir,
                                                              GError **error);
static gboolean gvir_sandbox_builder_clean_post_stop_default(GVirSandboxBuilder *builder,
                                                             GVirSandboxConfig *config,
                                                             const gchar *statedir,
                                                             GError **error);
static GList *gvir_sandbox_builder_get_files_to_copy(GVirSandboxBuilder *builder,
                                                     GVirSandboxConfig *config);

static void gvir_sandbox_builder_get_property(GObject *object,
                                              guint prop_id,
                                              GValue *value,
                                              GParamSpec *pspec)
{
    GVirSandboxBuilder *ctxt = GVIR_SANDBOX_BUILDER(object);
    GVirSandboxBuilderPrivate *priv = ctxt->priv;

    switch (prop_id) {
    case PROP_CONNECTION:
        g_value_set_object(value, priv->connection);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_builder_set_property(GObject *object,
                                              guint prop_id,
                                              const GValue *value,
                                              GParamSpec *pspec)
{
    GVirSandboxBuilder *ctxt = GVIR_SANDBOX_BUILDER(object);
    GVirSandboxBuilderPrivate *priv = ctxt->priv;

    switch (prop_id) {
    case PROP_CONNECTION:
        if (priv->connection)
            g_object_unref(priv->connection);
        priv->connection = g_value_dup_object(value);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_builder_finalize(GObject *object)
{
    GVirSandboxBuilder *ctxt = GVIR_SANDBOX_BUILDER(object);
    GVirSandboxBuilderPrivate *priv = ctxt->priv;

    if (priv->connection)
        g_object_unref(priv->connection);

    G_OBJECT_CLASS(gvir_sandbox_builder_parent_class)->finalize(object);
}


static void gvir_sandbox_builder_class_init(GVirSandboxBuilderClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = gvir_sandbox_builder_finalize;
    object_class->get_property = gvir_sandbox_builder_get_property;
    object_class->set_property = gvir_sandbox_builder_set_property;

    klass->construct_domain = gvir_sandbox_builder_construct_domain;
    klass->construct_basic = gvir_sandbox_builder_construct_basic;
    klass->construct_os = gvir_sandbox_builder_construct_os;
    klass->construct_features = gvir_sandbox_builder_construct_features;
    klass->construct_devices = gvir_sandbox_builder_construct_devices;
    klass->construct_security = gvir_sandbox_builder_construct_security;
    klass->clean_post_start = gvir_sandbox_builder_clean_post_start_default;
    klass->clean_post_stop = gvir_sandbox_builder_clean_post_stop_default;
    klass->get_files_to_copy = gvir_sandbox_builder_get_files_to_copy;

    g_object_class_install_property(object_class,
                                    PROP_CONNECTION,
                                    g_param_spec_object("connection",
                                                        "Connection",
                                                        "The sandbox connection",
                                                        GVIR_TYPE_CONNECTION,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_type_class_add_private(klass, sizeof(GVirSandboxBuilderPrivate));
}


static void gvir_sandbox_builder_init(GVirSandboxBuilder *ctxt)
{
    ctxt->priv = GVIR_SANDBOX_BUILDER_GET_PRIVATE(ctxt);
}


/**
 * gvir_sandbox_builder_for_connection:
 * @connection: (transfer none): the connection to host the sandbox
 * @error: (out): the error
 *
 * Find and instantiate a suitable builder for sandboxes to be hosted
 * under the @connection.
 *
 * Returns: (transfer full): a new builder or NULL
 */
GVirSandboxBuilder *gvir_sandbox_builder_for_connection(GVirConnection *connection,
                                                        GError **error)
{
    const gchar *uri = gvir_connection_get_uri(connection);
    GVirSandboxBuilder *builder = NULL;

    if (g_str_equal(uri, "lxc:///")) {
        builder = GVIR_SANDBOX_BUILDER(gvir_sandbox_builder_container_new(connection));
    } else if (g_str_equal(uri, "qemu:///session") ||
               g_str_equal(uri, "qemu:///system")) {
        builder = GVIR_SANDBOX_BUILDER(gvir_sandbox_builder_machine_new(connection));
    }

    if (!builder)
        *error = g_error_new(GVIR_SANDBOX_BUILDER_ERROR, 0,
                             "No builder available for URI %s", uri);

    return builder;
}


/**
 * gvir_sandbox_builder_get_connection:
 * @builder: (transfer none): the sandbox builder
 *
 * Retrieves the sandbox connection
 *
 * Returns: (transfer full): the current connection
 */
GVirConnection *gvir_sandbox_builder_get_connection(GVirSandboxBuilder *builder)
{
    GVirSandboxBuilderPrivate *priv = builder->priv;
    g_object_ref(priv->connection);
    return priv->connection;
}


static gboolean gvir_sandbox_builder_copy_file(const char *path,
                                               const char *libsdir,
                                               const char *newname,
                                               GError **error)
{
    gchar *name = g_path_get_basename(path);
    gchar *target = g_build_filename(libsdir, newname ? newname : name, NULL);
    GFile *srcFile = g_file_new_for_path(path);
    GFile *tgtFile = g_file_new_for_path(target);
    gboolean result = FALSE;


    if (!g_file_query_exists(tgtFile, NULL) &&
        !g_file_copy(srcFile, tgtFile, 0, NULL, NULL, NULL, error))
        goto cleanup;

    result = TRUE;

 cleanup:
    g_object_unref(tgtFile);
    g_object_unref(srcFile);
    g_free(target);
    g_free(name);

    return result;
}

static gboolean gvir_sandbox_builder_copy_program(const char *program,
                                                  const char *dest,
                                                  GError **error)
{
    gchar *out = NULL;
    gchar *line, *tmp;
    const gchar *argv[] = {LDD_PATH, program, NULL};
    gboolean result = FALSE;

    if (!gvir_sandbox_builder_copy_file(program, dest, NULL, error))
        goto cleanup;


    /* Get all the dependencies to be hard linked */
    if (!g_spawn_sync(NULL, (gchar **)argv, NULL, 0,
                      NULL, NULL, &out, NULL, NULL, error))
        goto cleanup;

    /* Loop over the output lines to get the path to the libraries to copy */
    line = out;
    while ((tmp = strchr(line, '\n'))) {
        gchar *start, *end;
        *tmp = '\0';

        /* Search the line for the library path */
        start = strstr(line, "/");
        end = strstr(line, " (");

        if (start && end) {
            const gchar *newname = NULL;
            *end = '\0';

            /* There are countless different naming schemes for
             * the ld-linux.so library across architectures. Pretty
             * much the only thing in common is they start with
             * the two letters 'ld'. The LDD program prints it
             * out differently too - it doesn't include " => "
             * as this library is special - its actually a static
             * linked executable not a library.
             *
             * To make life easier for libvirt-sandbox-init-{qemu,lxc}
             * we just call the file 'ld.so' when we copy it into our
             * scratch dir, no matter what it was called on the host.
             */
            if (!strstr(line, " => ") &&
                strstr(start, "/ld")) {
                newname = "ld.so";
            }

            if (!gvir_sandbox_builder_copy_file(start, dest, newname, error))
                goto cleanup;
        }

        line = tmp + 1;
    }
    result = TRUE;

 cleanup:
    g_free(out);

    return result;
}

static gboolean gvir_sandbox_builder_copy_init(GVirSandboxBuilder *builder,
                                               GVirSandboxConfig *config,
                                               const gchar *statedir,
                                               GError **error)
{
    gchar *libsdir;
    GVirSandboxBuilderClass *klass = GVIR_SANDBOX_BUILDER_GET_CLASS(builder);
    GList *tocopy = NULL, *tmp = NULL;
    gboolean result = FALSE;

    libsdir = g_build_filename(statedir, "config", ".libs", NULL);
    g_mkdir_with_parents(libsdir, 0755);

    tmp = tocopy = klass->get_files_to_copy(builder, config);
    while (tmp) {
        if (!gvir_sandbox_builder_copy_program(tmp->data, libsdir, error))
            goto cleanup;

        tmp = tmp->next;
    }
    result = TRUE;

 cleanup:
    g_free(libsdir);
    g_list_free_full(tocopy, g_free);

    return result;
}


static gboolean gvir_sandbox_builder_construct_domain(GVirSandboxBuilder *builder,
                                                      GVirSandboxConfig *config,
                                                      const gchar *statedir,
                                                      GVirConfigDomain *domain,
                                                      GError **error)
{
    GVirSandboxBuilderClass *klass = GVIR_SANDBOX_BUILDER_GET_CLASS(builder);

    if (!gvir_sandbox_builder_copy_init(builder, config, statedir, error))
        return FALSE;

    if (!(klass->construct_basic(builder, config, statedir, domain, error)))
        return FALSE;

    if (!(klass->construct_os(builder, config, statedir, domain, error)))
        return FALSE;

    if (!(klass->construct_features(builder, config, statedir, domain, error)))
        return FALSE;

    if (!(klass->construct_devices(builder, config, statedir, domain, error)))
        return FALSE;

    if (!(klass->construct_security(builder, config, statedir, domain, error)))
        return FALSE;

    return TRUE;
}


static gboolean gvir_sandbox_builder_construct_basic(GVirSandboxBuilder *builder G_GNUC_UNUSED,
                                                     GVirSandboxConfig *config,
                                                     const gchar *statedir G_GNUC_UNUSED,
                                                     GVirConfigDomain *domain,
                                                     GError **error G_GNUC_UNUSED)
{
    gvir_config_domain_set_name(domain,
                                gvir_sandbox_config_get_name(config));
#if 0
    /* Missing API in libvirt-gconfig */
    gvir_config_domain_set_uuid(domain,
                                gvir_sandbox_config_get_uuid(config));
#endif
    /* XXX configurable */
    gvir_config_domain_set_memory(domain, 1024*512);
    return TRUE;
}


static gboolean gvir_sandbox_builder_construct_os(GVirSandboxBuilder *builder G_GNUC_UNUSED,
                                                  GVirSandboxConfig *config G_GNUC_UNUSED,
                                                  const gchar *statedir G_GNUC_UNUSED,
                                                  GVirConfigDomain *domain G_GNUC_UNUSED,
                                                  GError **error G_GNUC_UNUSED)
{
    return TRUE;
}


static gboolean gvir_sandbox_builder_construct_features(GVirSandboxBuilder *builder G_GNUC_UNUSED,
                                                        GVirSandboxConfig *config G_GNUC_UNUSED,
                                                        const gchar *statedir G_GNUC_UNUSED,
                                                        GVirConfigDomain *domain G_GNUC_UNUSED,
                                                        GError **error G_GNUC_UNUSED)
{
    return TRUE;
}

static gboolean gvir_sandbox_builder_construct_disk_cfg(GVirSandboxBuilder *builder,
                                                        GVirSandboxConfig *config,
                                                        const gchar *statedir,
                                                        GError **error)
{
    GVirSandboxBuilderClass *klass = GVIR_SANDBOX_BUILDER_GET_CLASS(builder);
    guint nVirtioDev = 0;
    gchar *dskfile = g_strdup_printf("%s/config/disks.cfg", statedir);
    GFile *file = g_file_new_for_path(dskfile);
    GFileOutputStream *fos = g_file_replace(file,
                                            NULL,
                                            FALSE,
                                            G_FILE_CREATE_REPLACE_DESTINATION,
                                            NULL,
                                            error);
    gboolean ret = FALSE;
    GList *disks = gvir_sandbox_config_get_disks(config);
    GList *tmp = NULL;
    const gchar *tag;

    if (!fos)
        goto cleanup;

    tmp = disks;
    while (tmp) {
        GVirSandboxConfigDisk *mconfig = GVIR_SANDBOX_CONFIG_DISK(tmp->data);
        const gchar *prefix = klass->get_disk_prefix(builder, config, mconfig);
        gchar *device = g_strdup_printf("/dev/%s%c", prefix,
                                        (char)('a' + (nVirtioDev)++));
        gchar *line;

        tag = gvir_sandbox_config_disk_get_tag(mconfig);

        line = g_strdup_printf("%s\t%s\n",
                               tag, device);
        g_free(device);

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

    ret = TRUE;
 cleanup:
    g_list_foreach(disks, (GFunc)g_object_unref, NULL);
    g_list_free(disks);
    if (fos)
        g_object_unref(fos);
    if (!ret)
        g_file_delete(file, NULL, NULL);
    g_object_unref(file);
    g_free(dskfile);
    return ret;

}

static gboolean gvir_sandbox_builder_construct_devices(GVirSandboxBuilder *builder,
                                                       GVirSandboxConfig *config,
                                                       const gchar *statedir,
                                                       GVirConfigDomain *domain,
                                                       GError **error)
{
    return gvir_sandbox_builder_construct_disk_cfg(builder, config, statedir,error);
}

static gboolean gvir_sandbox_builder_construct_security_selinux (GVirSandboxBuilder *builder,
                                                                 GVirSandboxConfig *config,
                                                                 GVirConfigDomain *domain,
                                                                 GError **error)
{
    GVirConfigDomainSeclabel *sec = gvir_config_domain_seclabel_new();
    const char *label = gvir_sandbox_config_get_security_label(config);

    gvir_config_domain_seclabel_set_model(sec, "selinux");
    if (gvir_sandbox_config_get_security_dynamic(config)) {
        gvir_config_domain_seclabel_set_type(sec,
                                             GVIR_CONFIG_DOMAIN_SECLABEL_DYNAMIC);
        if (label)
            gvir_config_domain_seclabel_set_baselabel(sec, label);
        else if (gvir_config_domain_get_virt_type(domain) ==
                 GVIR_CONFIG_DOMAIN_VIRT_LXC)
            gvir_config_domain_seclabel_set_baselabel(sec, "system_u:system_r:svirt_lxc_net_t:s0");
        else if (gvir_config_domain_get_virt_type(domain) ==
                 GVIR_CONFIG_DOMAIN_VIRT_QEMU)
            gvir_config_domain_seclabel_set_baselabel(sec, "system_u:system_r:svirt_tcg_t:s0");
        else if (gvir_config_domain_get_virt_type(domain) ==
                 GVIR_CONFIG_DOMAIN_VIRT_KVM)
            gvir_config_domain_seclabel_set_baselabel(sec, "system_u:system_r:svirt_t:s0");
    } else {
        gvir_config_domain_seclabel_set_type(sec,
                                             GVIR_CONFIG_DOMAIN_SECLABEL_STATIC);
        if (label)
            gvir_config_domain_seclabel_set_label(sec, label);
    }

    gvir_config_domain_set_seclabel(domain, sec);
    g_object_unref(sec);

    return TRUE;
}

static gboolean gvir_sandbox_builder_construct_security_apparmor(GVirSandboxBuilder *builder,
                                                                 GVirSandboxConfig *config,
                                                                 GVirConfigDomain *domain,
                                                                 GError **error)
{
    GVirConfigDomainSeclabel *sec = gvir_config_domain_seclabel_new();
    const char *label = gvir_sandbox_config_get_security_label(config);

    gvir_config_domain_seclabel_set_model(sec, "apparmor");
    if (gvir_sandbox_config_get_security_dynamic(config)) {
        gvir_config_domain_seclabel_set_type(sec,
                                             GVIR_CONFIG_DOMAIN_SECLABEL_DYNAMIC);
    } else {
        gvir_config_domain_seclabel_set_type(sec,
                                             GVIR_CONFIG_DOMAIN_SECLABEL_STATIC);
        if (label)
            gvir_config_domain_seclabel_set_label(sec, label);
    }

    gvir_config_domain_set_seclabel(domain, sec);
    g_object_unref(sec);

    return TRUE;
}

static gboolean gvir_sandbox_builder_construct_security(GVirSandboxBuilder *builder,
                                                        GVirSandboxConfig *config,
                                                        const gchar *statedir G_GNUC_UNUSED,
                                                        GVirConfigDomain *domain,
                                                        GError **error)
{
    GVirConnection *connection = gvir_sandbox_builder_get_connection(builder);
    GVirConfigCapabilities *configCapabilities;
    GVirConfigCapabilitiesHost *hostCapabilities;
    GList *secmodels, *iter;
    gboolean supportsSelinux = FALSE;
    gboolean supportsAppArmor = FALSE;

    /* What security models are available on the host? */
    if (!(configCapabilities = gvir_connection_get_capabilities(connection, error))) {
        g_object_unref(connection);
        return FALSE;
    }

    hostCapabilities = gvir_config_capabilities_get_host(configCapabilities);

    secmodels = gvir_config_capabilities_host_get_secmodels(hostCapabilities);
    for (iter = secmodels; iter != NULL; iter = iter->next) {
        if (g_str_equal(gvir_config_capabilities_host_secmodel_get_model(
                GVIR_CONFIG_CAPABILITIES_HOST_SECMODEL(iter->data)), "selinux"))
            supportsSelinux = TRUE;
        if (g_str_equal(gvir_config_capabilities_host_secmodel_get_model(
                GVIR_CONFIG_CAPABILITIES_HOST_SECMODEL(iter->data)), "apparmor"))
            supportsAppArmor = TRUE;
        g_object_unref(iter->data);
    }

    g_list_free(secmodels);
    g_object_unref(hostCapabilities);
    g_object_unref(configCapabilities);
    g_object_unref(connection);

    if (supportsSelinux)
        return gvir_sandbox_builder_construct_security_selinux(builder, config,
                                                               domain, error);
    else if (supportsAppArmor)
        return gvir_sandbox_builder_construct_security_apparmor(builder, config,
                                                                domain, error);

    return TRUE;
}


static gboolean gvir_sandbox_builder_clean_post_start_default(GVirSandboxBuilder *builder G_GNUC_UNUSED,
                                                              GVirSandboxConfig *config G_GNUC_UNUSED,
                                                              const gchar *statedir G_GNUC_UNUSED,
                                                              GError **error G_GNUC_UNUSED)
{
    return TRUE;
}

static gboolean gvir_sandbox_builder_clean_post_stop_default(GVirSandboxBuilder *builder G_GNUC_UNUSED,
                                                             GVirSandboxConfig *config G_GNUC_UNUSED,
                                                             const gchar *statedir G_GNUC_UNUSED,
                                                             GError **error G_GNUC_UNUSED)
{
    return TRUE;
}

static GList *gvir_sandbox_builder_get_files_to_copy(GVirSandboxBuilder *builder,
                                                     GVirSandboxConfig *config G_GNUC_UNUSED)
{
    GList *tocopy = NULL;
    gchar *file = g_strdup_printf("%s/libvirt-sandbox-init-common", LIBEXECDIR);
    return g_list_append(tocopy, file);
}


/**
 * gvir_sandbox_builder_construct:
 * @builder: (transfer none): the sandbox builder
 * @config: (transfer none): the sandbox configuration
 * @error: (out): the error location
 *
 * Create a domain configuration from the sandbox configuration
 *
 * Returns: (transfer full): the newly built domain configuration
 */
GVirConfigDomain *gvir_sandbox_builder_construct(GVirSandboxBuilder *builder,
                                                 GVirSandboxConfig *config,
                                                 const gchar *statedir,
                                                 GError **error)
{
    GVirConfigDomain *domain = gvir_config_domain_new();
    GVirSandboxBuilderClass *klass = GVIR_SANDBOX_BUILDER_GET_CLASS(builder);

    if (!(klass->construct_domain(builder, config, statedir, domain, error))) {
        g_object_unref(domain);
        return NULL;
    }

    return domain;
}


/**
 * gvir_sandbox_builder_clean_post_start:
 * @builder: (transfer none): the sandbox builder
 * @config: (transfer none): the sandbox configuration
 * @error: (out): the error location
 *
 * Cleanup temporary files which are not required once the sandbox
 * has been started.
 *
 * Returns: TRUE on success, FALSE on error
 */
gboolean gvir_sandbox_builder_clean_post_start(GVirSandboxBuilder *builder,
                                               GVirSandboxConfig *config,
                                               const gchar *statedir,
                                               GError **error)
{
    GVirSandboxBuilderClass *klass = GVIR_SANDBOX_BUILDER_GET_CLASS(builder);

    return klass->clean_post_start(builder, config, statedir, error);
}


/**
 * gvir_sandbox_builder_clean_post_stop:
 * @builder: (transfer none): the sandbox builder
 * @config: (transfer none): the sandbox configuration
 * @error: (out): the error location
 *
 * Cleanup temporary files which are not required once the sandbox
 * has been started.
 *
 * Returns: TRUE on success, FALSE on error
 */
gboolean gvir_sandbox_builder_clean_post_stop(GVirSandboxBuilder *builder,
                                              GVirSandboxConfig *config,
                                              const gchar *statedir,
                                              GError **error)
{
    GVirSandboxBuilderClass *klass = GVIR_SANDBOX_BUILDER_GET_CLASS(builder);
    gchar *libsdir = g_build_filename(statedir, "config", ".libs", NULL);
    GFile *libsFile = g_file_new_for_path(libsdir);
    GFileEnumerator *enumerator = NULL;
    GFileInfo *info = NULL;
    GFile *child = NULL;
    gchar *dskfile = g_build_filename(statedir, "config", "disks.cfg", NULL);
    gboolean ret = TRUE;

    ret = klass->clean_post_stop(builder, config, statedir, error);

    if (unlink(dskfile) < 0 &&
        errno != ENOENT)
        ret = FALSE;

    if ((enumerator = g_file_enumerate_children(libsFile, "*", G_FILE_QUERY_INFO_NONE,
                                               NULL, error))) {
        while ((info = g_file_enumerator_next_file(enumerator, NULL, error))) {
            child = g_file_enumerator_get_child(enumerator, info);
            if (!g_file_delete(child, NULL, error))
                ret = FALSE;
            g_object_unref(child);
            child = NULL;
            g_object_unref(info);
            info = NULL;
        }

        if (!g_file_enumerator_close(enumerator, NULL, error))
            ret = FALSE;
    } else {
        if ((*error)->code != G_IO_ERROR_NOT_FOUND) {
            ret = FALSE;
            goto cleanup;
        }
        g_clear_error(error);
    }

    if (!g_file_delete(libsFile, NULL, error) &&
        (*error)->code != G_IO_ERROR_NOT_FOUND)
        ret = FALSE;

 cleanup:
    if (enumerator)
        g_object_unref(enumerator);
    g_object_unref(libsFile);
    g_free(libsdir);
    g_free(dskfile);
    return ret;
}


void gvir_sandbox_builder_set_filterref(GVirSandboxBuilder *builder,
                                        GVirConfigDomainInterface *iface,
                                        GVirSandboxConfigNetworkFilterref *filterref)
{
    GVirConfigDomainInterfaceFilterref *glib_fref;

    GList *param_iter, *parameters;
    const gchar *fref_name = gvir_sandbox_config_network_filterref_get_name(filterref);

    glib_fref = gvir_config_domain_interface_filterref_new();
    gvir_config_domain_interface_filterref_set_name(glib_fref, fref_name);
    param_iter = parameters = gvir_sandbox_config_network_filterref_get_parameters(filterref);
    while (param_iter) {
        const gchar *name;
        const gchar *value;
        GVirSandboxConfigNetworkFilterrefParameter *param = GVIR_SANDBOX_CONFIG_NETWORK_FILTERREF_PARAMETER(param_iter->data);
        GVirConfigDomainInterfaceFilterrefParameter *glib_param;

        name = gvir_sandbox_config_network_filterref_parameter_get_name(param);
        value = gvir_sandbox_config_network_filterref_parameter_get_value(param);

        glib_param = gvir_config_domain_interface_filterref_parameter_new();
        gvir_config_domain_interface_filterref_parameter_set_name(glib_param, name);
        gvir_config_domain_interface_filterref_parameter_set_value(glib_param, value);

        gvir_config_domain_interface_filterref_add_parameter(glib_fref, glib_param);
        g_object_unref(glib_param);

        param_iter = param_iter->next;
    }

    g_list_foreach(parameters, (GFunc)g_object_unref, NULL);
    g_list_free(parameters);

    gvir_config_domain_interface_set_filterref(iface, glib_fref);
    g_object_unref(glib_fref);
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
