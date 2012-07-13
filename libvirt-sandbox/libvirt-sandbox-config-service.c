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
    GList *units;
};

G_DEFINE_TYPE(GVirSandboxConfigService, gvir_sandbox_config_service, GVIR_SANDBOX_TYPE_CONFIG);


enum {
    PROP_0,
};

enum {
    LAST_SIGNAL
};

//static gint signals[LAST_SIGNAL];

#define GVIR_SANDBOX_CONFIG_SERVICE_ERROR gvir_sandbox_config_service_error_quark()

static GQuark
gvir_sandbox_config_service_error_quark(void)
{
    return g_quark_from_static_string("gvir-sandbox-config-service");
}

static void gvir_sandbox_config_service_get_property(GObject *object,
                                                     guint prop_id,
                                                     GValue *value,
                                                     GParamSpec *pspec)
{
    switch (prop_id) {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_config_service_set_property(GObject *object,
                                                     guint prop_id,
                                                     const GValue *value,
                                                     GParamSpec *pspec)
{
    switch (prop_id) {
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
    gint i;
    gchar *key;
    GError *e = NULL;

    if (!GVIR_SANDBOX_CONFIG_CLASS(gvir_sandbox_config_service_parent_class)
        ->load_config(config, file, error))
        goto cleanup;

    for (i = 0 ; i < 1024 ; i++) {
        key = g_strdup_printf("unit.%u", i);
        gchar *name;
        if ((name = g_key_file_get_string(file, key, "name", &e)) == NULL) {
            g_free(key);
            if (e->code == G_KEY_FILE_ERROR_GROUP_NOT_FOUND) {
                g_error_free(e);
                break;
            }
            g_error_free(e);
            g_set_error(error, GVIR_SANDBOX_CONFIG_SERVICE_ERROR, 0,
                        "%s", "Missing unit name in config file");
            goto cleanup;
        }
        priv->units = g_list_append(priv->units, name);
        g_free(key);
    }

    ret = TRUE;

cleanup:
    return ret;
}


static void gvir_sandbox_config_service_save_config(GVirSandboxConfig *config,
                                                    GKeyFile *file)
{
    GVirSandboxConfigServicePrivate *priv = GVIR_SANDBOX_CONFIG_SERVICE(config)->priv;
    GList *tmp;
    gint i;

    GVIR_SANDBOX_CONFIG_CLASS(gvir_sandbox_config_service_parent_class)
        ->save_config(config, file);

    tmp = priv->units;
    i = 0;
    while (tmp) {
        gchar *key = g_strdup_printf("unit.%u", i);
        g_key_file_set_string(file, key, "name", tmp->data);
        g_free(key);
        tmp = tmp->next;
    }

}


static void gvir_sandbox_config_service_finalize(GObject *object)
{
    GVirSandboxConfigService *config = GVIR_SANDBOX_CONFIG_SERVICE(object);
    GVirSandboxConfigServicePrivate *priv = config->priv;

    g_list_foreach(priv->units, (GFunc)g_free, NULL);
    g_list_free(priv->units);

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
 * gvir_sandbox_config_service_get_units:
 *
 * Returns: (transfer container) (element-type utf8): a list of units
 */
GList *gvir_sandbox_config_service_get_units(GVirSandboxConfigService *config)
{
    GVirSandboxConfigServicePrivate *priv = config->priv;
    return g_list_copy(priv->units);
}


void gvir_sandbox_config_service_add_unit(GVirSandboxConfigService *config,
                                          const gchar *unit)
{
    GVirSandboxConfigServicePrivate *priv = config->priv;
    priv->units = g_list_append(priv->units, g_strdup(unit));
}
