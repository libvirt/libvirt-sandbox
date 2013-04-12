/*
 * libvirt-sandbox-config-service-systemd.c: libvirt sandbox configuration
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
 * SECTION: libvirt-sandbox-config-service-systemd
 * @short_description: ServiceSystemd sandbox configuration details
 * @include: libvirt-sandbox/libvirt-sandbox.h
 * @see_also: #GVirSandboxConfigServiceSystemd
 *
 * Provides an object to store configuration details for a systemd service config
 *
 * The GVirSandboxConfigServiceSystemd object extends #GVirSandboxConfigService to store
 * the extra information required to setup a service sandbox with systemd
 */

#define GVIR_SANDBOX_CONFIG_SERVICE_SYSTEMD_GET_PRIVATE(obj)                         \
        (G_TYPE_INSTANCE_GET_PRIVATE((obj), GVIR_SANDBOX_TYPE_CONFIG_SERVICE_SYSTEMD, GVirSandboxConfigServiceSystemdPrivate))

struct _GVirSandboxConfigServiceSystemdPrivate
{
    gchar *bootTarget;
};

G_DEFINE_TYPE(GVirSandboxConfigServiceSystemd, gvir_sandbox_config_service_systemd, GVIR_SANDBOX_TYPE_CONFIG_SERVICE);

static gchar **gvir_sandbox_config_service_systemd_get_command(GVirSandboxConfig *config);

enum {
    PROP_0,
    PROP_BOOT_TARGET,
};

enum {
    LAST_SIGNAL
};

//static gint signals[LAST_SIGNAL];

#if 0
#define GVIR_SANDBOX_CONFIG_SERVICE_SYSTEMD_ERROR gvir_sandbox_config_service_systemd_error_quark()

static GQuark
gvir_sandbox_config_service_systemd_error_quark(void)
{
    return g_quark_from_static_string("gvir-sandbox-config-service-systemd");
}
#endif

static void gvir_sandbox_config_service_systemd_get_property(GObject *object,
                                                             guint prop_id,
                                                             GValue *value,
                                                             GParamSpec *pspec)
{
    GVirSandboxConfigServiceSystemd *config = GVIR_SANDBOX_CONFIG_SERVICE_SYSTEMD(object);
    GVirSandboxConfigServiceSystemdPrivate *priv = config->priv;

    switch (prop_id) {
    case PROP_BOOT_TARGET:
        g_value_set_string(value, priv->bootTarget);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_config_service_systemd_set_property(GObject *object,
                                                             guint prop_id,
                                                             const GValue *value,
                                                             GParamSpec *pspec)
{
    GVirSandboxConfigServiceSystemd *config = GVIR_SANDBOX_CONFIG_SERVICE_SYSTEMD(object);
    GVirSandboxConfigServiceSystemdPrivate *priv = config->priv;

    switch (prop_id) {
    case PROP_BOOT_TARGET:
        g_free(priv->bootTarget);
        priv->bootTarget = g_value_dup_string(value);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static gboolean gvir_sandbox_config_service_systemd_load_config(GVirSandboxConfig *config,
                                                                GKeyFile *file,
                                                                GError **error)
{
    GVirSandboxConfigServiceSystemdPrivate *priv = GVIR_SANDBOX_CONFIG_SERVICE_SYSTEMD(config)->priv;
    gboolean ret = FALSE;

    if (!GVIR_SANDBOX_CONFIG_CLASS(gvir_sandbox_config_service_systemd_parent_class)
        ->load_config(config, file, error))
        goto cleanup;

    if ((priv->bootTarget = g_key_file_get_string(file, "service", "bootTarget", error)) == NULL)
        goto cleanup;

    ret = TRUE;

cleanup:
    return ret;
}


static void gvir_sandbox_config_service_systemd_save_config(GVirSandboxConfig *config,
                                                            GKeyFile *file)
{
    GVirSandboxConfigServiceSystemdPrivate *priv = GVIR_SANDBOX_CONFIG_SERVICE_SYSTEMD(config)->priv;

    GVIR_SANDBOX_CONFIG_CLASS(gvir_sandbox_config_service_systemd_parent_class)
        ->save_config(config, file);

    g_key_file_set_string(file, "service", "bootTarget", priv->bootTarget);
}


static void gvir_sandbox_config_service_systemd_finalize(GObject *object)
{
    GVirSandboxConfigServiceSystemd *config = GVIR_SANDBOX_CONFIG_SERVICE_SYSTEMD(object);
    GVirSandboxConfigServiceSystemdPrivate *priv = config->priv;

    g_free(priv->bootTarget);

    G_OBJECT_CLASS(gvir_sandbox_config_service_systemd_parent_class)->finalize(object);
}


static void gvir_sandbox_config_service_systemd_class_init(GVirSandboxConfigServiceSystemdClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    GVirSandboxConfigClass *config_class = GVIR_SANDBOX_CONFIG_CLASS(klass);

    object_class->finalize = gvir_sandbox_config_service_systemd_finalize;
    object_class->get_property = gvir_sandbox_config_service_systemd_get_property;
    object_class->set_property = gvir_sandbox_config_service_systemd_set_property;

    config_class->load_config = gvir_sandbox_config_service_systemd_load_config;
    config_class->save_config = gvir_sandbox_config_service_systemd_save_config;
    config_class->get_command = gvir_sandbox_config_service_systemd_get_command;

    g_type_class_add_private(klass, sizeof(GVirSandboxConfigServiceSystemdPrivate));
}


static void gvir_sandbox_config_service_systemd_init(GVirSandboxConfigServiceSystemd *config)
{
    config->priv = GVIR_SANDBOX_CONFIG_SERVICE_SYSTEMD_GET_PRIVATE(config);
}


/**
 * gvir_sandbox_config_service_systemd_new:
 * @name: the sandbox name
 *
 * Create a new service application sandbox configuration
 *
 * Returns: (transfer full): a new service sandbox config object
 */
GVirSandboxConfigServiceSystemd *gvir_sandbox_config_service_systemd_new(const gchar *name)
{
    return GVIR_SANDBOX_CONFIG_SERVICE_SYSTEMD(g_object_new(GVIR_SANDBOX_TYPE_CONFIG_SERVICE_SYSTEMD,
                                                            "name", name,
                                                            NULL));
}


/**
 * gvir_sandbox_config_service_systemd_get_boot_target:
 *
 * Returns: the boot target name
 */
const gchar *gvir_sandbox_config_service_systemd_get_boot_target(GVirSandboxConfigServiceSystemd *config)
{
    GVirSandboxConfigServiceSystemdPrivate *priv = config->priv;
    return priv->bootTarget;
}


void gvir_sandbox_config_service_systemd_set_boot_target(GVirSandboxConfigServiceSystemd *config,
                                                         const gchar *target)
{
    GVirSandboxConfigServiceSystemdPrivate *priv = config->priv;
    g_free(priv->bootTarget);
    priv->bootTarget = g_strdup(target);
}


static gchar **gvir_sandbox_config_service_systemd_get_command(GVirSandboxConfig *config)
{
    GVirSandboxConfigServiceSystemd *sconfig = GVIR_SANDBOX_CONFIG_SERVICE_SYSTEMD(config);
    GVirSandboxConfigServiceSystemdPrivate *priv = sconfig->priv;
    gchar **command = g_new(gchar *, 7);

    command[0] = g_strdup("/bin/systemd");
    command[1] = g_strdup("--unit");
    command[2] = g_strdup(priv->bootTarget);
    command[3] = g_strdup("--log-target");
    command[4] = g_strdup("console");
    command[5] = g_strdup("--system");
    command[6] = NULL;

    return command;
}
