/*
 * libvirt-sandbox-config-service.c: libvirt sandbox configuration
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
 * SECTION: libvirt-sandbox-config-service
 * @short_description: Service sandbox configuration details
 * @include: libvirt-sandbox/libvirt-sandbox.h
 * @see_also: #GVirSandboxConfigService
 *
 * Provides an object to store configuration details for a service config
 *
 * The GVirSandboxConfigService object extends #GVirSandboxConfig to store
 * the extra information required to setup a service sandbox
 */

#define GVIR_SANDBOX_CONFIG_SERVICE_GET_PRIVATE(obj)                         \
        (G_TYPE_INSTANCE_GET_PRIVATE((obj), GVIR_SANDBOX_TYPE_CONFIG_SERVICE, GVirSandboxConfigServicePrivate))

struct _GVirSandboxConfigServicePrivate
{
    gchar *bootTarget;
};

G_DEFINE_TYPE(GVirSandboxConfigService, gvir_sandbox_config_service, GVIR_SANDBOX_TYPE_CONFIG);


enum {
    PROP_0,
    PROP_BOOT_TARGET,
};

enum {
    LAST_SIGNAL
};

//static gint signals[LAST_SIGNAL];

#if 0
#define GVIR_SANDBOX_CONFIG_SERVICE_ERROR gvir_sandbox_config_service_error_quark()

static GQuark
gvir_sandbox_config_service_error_quark(void)
{
    return g_quark_from_static_string("gvir-sandbox-config-service");
}
#endif

static void gvir_sandbox_config_service_get_property(GObject *object,
                                                     guint prop_id,
                                                     GValue *value,
                                                     GParamSpec *pspec)
{
    GVirSandboxConfigService *config = GVIR_SANDBOX_CONFIG_SERVICE(object);
    GVirSandboxConfigServicePrivate *priv = config->priv;

    switch (prop_id) {
    case PROP_BOOT_TARGET:
        g_value_set_string(value, priv->bootTarget);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_config_service_set_property(GObject *object,
                                                     guint prop_id,
                                                     const GValue *value,
                                                     GParamSpec *pspec)
{
    GVirSandboxConfigService *config = GVIR_SANDBOX_CONFIG_SERVICE(object);
    GVirSandboxConfigServicePrivate *priv = config->priv;

    switch (prop_id) {
    case PROP_BOOT_TARGET:
        g_free(priv->bootTarget);
        priv->bootTarget = g_value_dup_string(value);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static gboolean gvir_sandbox_config_service_load_config(GVirSandboxConfig *config,
                                                        GKeyFile *file,
                                                        GError **error)
{
    GVirSandboxConfigServicePrivate *priv = GVIR_SANDBOX_CONFIG_SERVICE(config)->priv;
    gboolean ret = FALSE;

    if (!GVIR_SANDBOX_CONFIG_CLASS(gvir_sandbox_config_service_parent_class)
        ->load_config(config, file, error))
        goto cleanup;

    if ((priv->bootTarget = g_key_file_get_string(file, "service", "bootTarget", error)) == NULL)
        goto cleanup;

    ret = TRUE;

cleanup:
    return ret;
}


static void gvir_sandbox_config_service_save_config(GVirSandboxConfig *config,
                                                    GKeyFile *file)
{
    GVirSandboxConfigServicePrivate *priv = GVIR_SANDBOX_CONFIG_SERVICE(config)->priv;

    GVIR_SANDBOX_CONFIG_CLASS(gvir_sandbox_config_service_parent_class)
        ->save_config(config, file);

    g_key_file_set_string(file, "service", "bootTarget", priv->bootTarget);
}


static void gvir_sandbox_config_service_finalize(GObject *object)
{
    GVirSandboxConfigService *config = GVIR_SANDBOX_CONFIG_SERVICE(object);
    GVirSandboxConfigServicePrivate *priv = config->priv;

    g_free(priv->bootTarget);

    G_OBJECT_CLASS(gvir_sandbox_config_service_parent_class)->finalize(object);
}


static void gvir_sandbox_config_service_class_init(GVirSandboxConfigServiceClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    GVirSandboxConfigClass *config_class = GVIR_SANDBOX_CONFIG_CLASS(klass);

    object_class->finalize = gvir_sandbox_config_service_finalize;
    object_class->get_property = gvir_sandbox_config_service_get_property;
    object_class->set_property = gvir_sandbox_config_service_set_property;

    config_class->load_config = gvir_sandbox_config_service_load_config;
    config_class->save_config = gvir_sandbox_config_service_save_config;

    g_type_class_add_private(klass, sizeof(GVirSandboxConfigServicePrivate));
}


static void gvir_sandbox_config_service_init(GVirSandboxConfigService *config)
{
    config->priv = GVIR_SANDBOX_CONFIG_SERVICE_GET_PRIVATE(config);
}


/**
 * gvir_sandbox_config_service_new:
 * @name: the sandbox name
 *
 * Create a new service application sandbox configuration
 *
 * Returns: (transfer full): a new service sandbox config object
 */
GVirSandboxConfigService *gvir_sandbox_config_service_new(const gchar *name)
{
    return GVIR_SANDBOX_CONFIG_SERVICE(g_object_new(GVIR_SANDBOX_TYPE_CONFIG_SERVICE,
                                                    "name", name,
                                                    NULL));
}


/**
 * gvir_sandbox_config_service_get_boot_target:
 *
 * Returns: the boot target name
 */
const gchar *gvir_sandbox_config_service_get_boot_target(GVirSandboxConfigService *config)
{
    GVirSandboxConfigServicePrivate *priv = config->priv;
    return priv->bootTarget;
}


void gvir_sandbox_config_service_set_boot_target(GVirSandboxConfigService *config,
                                                 const gchar *target)
{
    GVirSandboxConfigServicePrivate *priv = config->priv;
    g_free(priv->bootTarget);
    priv->bootTarget = g_strdup(target);
}
