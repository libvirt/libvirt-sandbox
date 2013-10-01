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

#include "libvirt-sandbox/libvirt-sandbox.h"

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

#define GVIR_SANDBOX_BUILDER_GET_PRIVATE(obj)                         \
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


static gboolean gvir_sandbox_builder_construct_domain(GVirSandboxBuilder *builder,
                                                      GVirSandboxConfig *config,
                                                      const gchar *statedir,
                                                      GVirConfigDomain *domain,
                                                      GError **error)
{
    GVirSandboxBuilderClass *klass = GVIR_SANDBOX_BUILDER_GET_CLASS(builder);

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


static gboolean gvir_sandbox_builder_construct_devices(GVirSandboxBuilder *builder G_GNUC_UNUSED,
                                                       GVirSandboxConfig *config G_GNUC_UNUSED,
                                                       const gchar *statedir G_GNUC_UNUSED,
                                                       GVirConfigDomain *domain G_GNUC_UNUSED,
                                                       GError **error G_GNUC_UNUSED)
{
    return TRUE;
}


static gboolean gvir_sandbox_builder_construct_security(GVirSandboxBuilder *builder G_GNUC_UNUSED,
                                                        GVirSandboxConfig *config G_GNUC_UNUSED,
                                                        const gchar *statedir G_GNUC_UNUSED,
                                                        GVirConfigDomain *domain,
                                                        GError **error G_GNUC_UNUSED)
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

    return klass->clean_post_stop(builder, config, statedir, error);
}
