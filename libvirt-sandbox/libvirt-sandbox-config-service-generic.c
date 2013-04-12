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
 * SECTION: libvirt-sandbox-config-service-generic
 * @short_description: ServiceGeneric sandbox configuration details
 * @include: libvirt-sandbox/libvirt-sandbox.h
 * @see_also: #GVirSandboxConfigServiceGeneric
 *
 * Provides an object to store configuration details for a generic service config
 *
 * The GVirSandboxConfigServiceGeneric object extends #GVirSandboxConfigService to store
 * the extra information required to setup a service sandbox with generic
 */

#define GVIR_SANDBOX_CONFIG_SERVICE_GENERIC_GET_PRIVATE(obj)                         \
        (G_TYPE_INSTANCE_GET_PRIVATE((obj), GVIR_SANDBOX_TYPE_CONFIG_SERVICE_GENERIC, GVirSandboxConfigServiceGenericPrivate))

struct _GVirSandboxConfigServiceGenericPrivate
{
    gchar **command;
};

G_DEFINE_TYPE(GVirSandboxConfigServiceGeneric, gvir_sandbox_config_service_generic, GVIR_SANDBOX_TYPE_CONFIG_SERVICE);

static gchar **gvir_sandbox_config_service_generic_get_command(GVirSandboxConfig *config);

enum {
    PROP_0,
    PROP_BOOT_TARGET,
};

enum {
    LAST_SIGNAL
};

//static gint signals[LAST_SIGNAL];

#if 0
#define GVIR_SANDBOX_CONFIG_SERVICE_GENERIC_ERROR gvir_sandbox_config_service_generic_error_quark()

static GQuark
gvir_sandbox_config_service_generic_error_quark(void)
{
    return g_quark_from_static_string("gvir-sandbox-config-service-generic");
}
#endif


static gboolean gvir_sandbox_config_service_generic_load_config(GVirSandboxConfig *config,
                                                                GKeyFile *file,
                                                                GError **error)
{
    GVirSandboxConfigServiceGenericPrivate *priv = GVIR_SANDBOX_CONFIG_SERVICE_GENERIC(config)->priv;
    gboolean ret = FALSE;
    GError *e = NULL;
    gchar *str;
    gsize i;
    gchar **argv = g_new0(gchar *, 1);
    gsize argc = 0;
    argv[0] = NULL;

    if (!GVIR_SANDBOX_CONFIG_CLASS(gvir_sandbox_config_service_generic_parent_class)
        ->load_config(config, file, error))
        goto cleanup;

    for (i = 0 ; i < 1024 ; i++) {
        gchar *key = g_strdup_printf("argv.%zu", i);
        if ((str = g_key_file_get_string(file, "command", key, &e)) == NULL) {
            if (e->code == G_KEY_FILE_ERROR_KEY_NOT_FOUND) {
                g_error_free(e);
                e = NULL;
                break;
            }
            g_propagate_error(error, e);
            goto cleanup;
        }

        argv = g_renew(char *, argv, argc+2);
        argv[argc++] = str;
        argv[argc] = NULL;
    }
    g_strfreev(priv->command);
    priv->command = argv;
    argv = NULL;

    ret = TRUE;

cleanup:
    return ret;
}


static void gvir_sandbox_config_service_generic_save_config(GVirSandboxConfig *config,
                                                            GKeyFile *file)
{
    GVirSandboxConfigServiceGenericPrivate *priv = GVIR_SANDBOX_CONFIG_SERVICE_GENERIC(config)->priv;
    size_t i, argc;

    GVIR_SANDBOX_CONFIG_CLASS(gvir_sandbox_config_service_generic_parent_class)
        ->save_config(config, file);

    argc = g_strv_length(priv->command);
    for (i = 0 ; i < argc ; i++) {
        gchar *key = g_strdup_printf("argv.%zu", i);
        g_key_file_set_string(file, "command", key, priv->command[i]);
        g_free(key);
    }
}


static void gvir_sandbox_config_service_generic_finalize(GObject *object)
{
    GVirSandboxConfigServiceGeneric *config = GVIR_SANDBOX_CONFIG_SERVICE_GENERIC(object);
    GVirSandboxConfigServiceGenericPrivate *priv = config->priv;

    g_strfreev(priv->command);

    G_OBJECT_CLASS(gvir_sandbox_config_service_generic_parent_class)->finalize(object);
}


static void gvir_sandbox_config_service_generic_class_init(GVirSandboxConfigServiceGenericClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    GVirSandboxConfigClass *config_class = GVIR_SANDBOX_CONFIG_CLASS(klass);

    object_class->finalize = gvir_sandbox_config_service_generic_finalize;

    config_class->load_config = gvir_sandbox_config_service_generic_load_config;
    config_class->save_config = gvir_sandbox_config_service_generic_save_config;
    config_class->get_command = gvir_sandbox_config_service_generic_get_command;

    g_type_class_add_private(klass, sizeof(GVirSandboxConfigServiceGenericPrivate));
}


static void gvir_sandbox_config_service_generic_init(GVirSandboxConfigServiceGeneric *config)
{
    GVirSandboxConfigServiceGenericPrivate *priv;

    priv = config->priv = GVIR_SANDBOX_CONFIG_SERVICE_GENERIC_GET_PRIVATE(config);

    priv->command = g_new0(gchar *, 2);
    priv->command[0] = g_strdup("/bin/sh");
    priv->command[1] = NULL;
}


/**
 * gvir_sandbox_config_service_generic_new:
 * @name: the sandbox name
 *
 * Create a new service application sandbox configuration
 *
 * Returns: (transfer full): a new service sandbox config object
 */
GVirSandboxConfigServiceGeneric *gvir_sandbox_config_service_generic_new(const gchar *name)
{
    return GVIR_SANDBOX_CONFIG_SERVICE_GENERIC(g_object_new(GVIR_SANDBOX_TYPE_CONFIG_SERVICE_GENERIC,
                                                            "name", name,
                                                            NULL));
}


/**
 * gvir_sandbox_config_service_generic_set_command:
 * @config: (transfer none): the sandbox config
 * @argv: (transfer none)(array zero-terminated=1): the command path and arguments
 *
 * Set the path of the command to be run and its arguments. The @argv should
 * be a NULL terminated list
 */
void gvir_sandbox_config_service_generic_set_command(GVirSandboxConfigServiceGeneric *config, gchar **argv)
{
    GVirSandboxConfigServiceGenericPrivate *priv = config->priv;
    guint len = g_strv_length(argv);
    size_t i;
    g_strfreev(priv->command);
    priv->command = g_new0(gchar *, len + 1);
    for (i = 0 ; i < len ; i++)
        priv->command[i] = g_strdup(argv[i]);
    priv->command[len] = NULL;
}


static gchar **gvir_sandbox_config_service_generic_get_command(GVirSandboxConfig *config)
{
    GVirSandboxConfigServiceGeneric *iconfig = GVIR_SANDBOX_CONFIG_SERVICE_GENERIC(config);
    GVirSandboxConfigServiceGenericPrivate *priv = iconfig->priv;
    return g_strdupv(priv->command);
}
