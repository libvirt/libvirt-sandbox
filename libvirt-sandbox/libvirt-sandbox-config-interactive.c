/*
 * libvirt-sandbox-config-interactive.c: libvirt sandbox configuration
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
 * SECTION: libvirt-sandbox-config-interactive
 * @short_description: Interactive sandbox configuration details
 * @include: libvirt-sandbox/libvirt-sandbox.h
 * @see_also: #GVirSandboxConfigInteractive
 *
 * Provides an object to store configuration details for a interactive config
 *
 * The GVirSandboxConfigInteractive object extends #GVirSandboxConfig to store
 * the extra information required to setup a _interactive desktop application
 * sandbox.
 */

#define GVIR_SANDBOX_CONFIG_INTERACTIVE_GET_PRIVATE(obj)                         \
        (G_TYPE_INSTANCE_GET_PRIVATE((obj), GVIR_SANDBOX_TYPE_CONFIG_INTERACTIVE, GVirSandboxConfigInteractivePrivate))

struct _GVirSandboxConfigInteractivePrivate
{
    gboolean tty;
    gchar **command;
};

G_DEFINE_TYPE(GVirSandboxConfigInteractive, gvir_sandbox_config_interactive, GVIR_SANDBOX_TYPE_CONFIG);

static gchar **gvir_sandbox_config_interactive_get_command(GVirSandboxConfig *config);

enum {
    PROP_0,
    PROP_TTY,
};

enum {
    LAST_SIGNAL
};

//static gint signals[LAST_SIGNAL];


static void gvir_sandbox_config_interactive_get_property(GObject *object,
                                                         guint prop_id,
                                                         GValue *value,
                                                         GParamSpec *pspec)
{
    GVirSandboxConfigInteractive *config = GVIR_SANDBOX_CONFIG_INTERACTIVE(object);
    GVirSandboxConfigInteractivePrivate *priv = config->priv;

    switch (prop_id) {
    case PROP_TTY:
        g_value_set_boolean(value, priv->tty);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_config_interactive_set_property(GObject *object,
                                                         guint prop_id,
                                                         const GValue *value,
                                                         GParamSpec *pspec)
{
    GVirSandboxConfigInteractive *config = GVIR_SANDBOX_CONFIG_INTERACTIVE(object);
    GVirSandboxConfigInteractivePrivate *priv = config->priv;

    switch (prop_id) {

    default:
    case PROP_TTY:
        priv->tty = g_value_get_boolean(value);
        break;

        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static gboolean gvir_sandbox_config_interactive_load_config(GVirSandboxConfig *config,
                                                            GKeyFile *file,
                                                            GError **error)
{
    GVirSandboxConfigInteractivePrivate *priv = GVIR_SANDBOX_CONFIG_INTERACTIVE(config)->priv;
    gboolean ret = FALSE;
    size_t i;
    gchar *str;
    gboolean b;
    GError *e = NULL;
    gchar **argv = g_new0(gchar *, 1);
    gsize argc = 0;
    argv[0] = NULL;

    if (!GVIR_SANDBOX_CONFIG_CLASS(gvir_sandbox_config_interactive_parent_class)
        ->load_config(config, file, error))
        goto cleanup;

    b = g_key_file_get_boolean(file, "interactive", "tty", &e);
    if (e) {
        g_error_free(e);
        e = NULL;
    } else {
        priv->tty = b;
    }

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
    if (argv)
        g_strfreev(argv);
    return ret;
}


static void gvir_sandbox_config_interactive_save_config(GVirSandboxConfig *config,
                                                        GKeyFile *file)
{
    GVirSandboxConfigInteractivePrivate *priv = GVIR_SANDBOX_CONFIG_INTERACTIVE(config)->priv;
    size_t i, argc;

    GVIR_SANDBOX_CONFIG_CLASS(gvir_sandbox_config_interactive_parent_class)
        ->save_config(config, file);

    g_key_file_set_boolean(file, "interactive", "tty", priv->tty);

    argc = g_strv_length(priv->command);
    for (i = 0 ; i < argc ; i++) {
        gchar *key = g_strdup_printf("argv.%zu", i);
        g_key_file_set_string(file, "command", key, priv->command[i]);
        g_free(key);
    }
}


static void gvir_sandbox_config_interactive_finalize(GObject *object)
{
    GVirSandboxConfigInteractive *config = GVIR_SANDBOX_CONFIG_INTERACTIVE(object);
    GVirSandboxConfigInteractivePrivate *priv = config->priv;

    g_strfreev(priv->command);

    G_OBJECT_CLASS(gvir_sandbox_config_interactive_parent_class)->finalize(object);
}


static void gvir_sandbox_config_interactive_class_init(GVirSandboxConfigInteractiveClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    GVirSandboxConfigClass *config_class = GVIR_SANDBOX_CONFIG_CLASS(klass);

    object_class->finalize = gvir_sandbox_config_interactive_finalize;
    object_class->get_property = gvir_sandbox_config_interactive_get_property;
    object_class->set_property = gvir_sandbox_config_interactive_set_property;

    config_class->load_config = gvir_sandbox_config_interactive_load_config;
    config_class->save_config = gvir_sandbox_config_interactive_save_config;
    config_class->get_command = gvir_sandbox_config_interactive_get_command;

    g_object_class_install_property(object_class,
                                    PROP_TTY,
                                    g_param_spec_string("tty",
                                                        "TTY",
                                                        "TTY",
                                                        FALSE,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_type_class_add_private(klass, sizeof(GVirSandboxConfigInteractivePrivate));
}


static void gvir_sandbox_config_interactive_init(GVirSandboxConfigInteractive *config)
{
    GVirSandboxConfigInteractivePrivate *priv;

    priv = config->priv = GVIR_SANDBOX_CONFIG_INTERACTIVE_GET_PRIVATE(config);

    priv->command = g_new0(gchar *, 2);
    priv->command[0] = g_strdup("/bin/sh");
    priv->command[1] = NULL;
}


/**
 * gvir_sandbox_config_interactive_new:
 * @name: the sandbox name
 *
 * Create a new interactive application sandbox configuration
 *
 * Returns: (transfer full): a new interactive sandbox config object
 */
GVirSandboxConfigInteractive *gvir_sandbox_config_interactive_new(const gchar *name)
{
    return GVIR_SANDBOX_CONFIG_INTERACTIVE(g_object_new(GVIR_SANDBOX_TYPE_CONFIG_INTERACTIVE,
                                                        "name", name,
                                                        NULL));
}


/**
 * gvir_sandbox_config_interactive_set_tty:
 * @config: (transfer none): the sandbox config
 * @tty: (transfer none): true if the container should have a tty
 *
 * Set whether the container console should have a interactive tty.
 */
void gvir_sandbox_config_interactive_set_tty(GVirSandboxConfigInteractive *config, gboolean tty)
{
    GVirSandboxConfigInteractivePrivate *priv = config->priv;
    priv->tty = tty;
}


/**
 * gvir_sandbox_config_interactive_get_tty:
 * @config: (transfer none): the sandbox config
 *
 * Retrieves the sandbox tty flag
 *
 * Returns: (transfer none): the tty flag
 */
gboolean gvir_sandbox_config_interactive_get_tty(GVirSandboxConfigInteractive *config)
{
    GVirSandboxConfigInteractivePrivate *priv = config->priv;
    return priv->tty;
}


/**
 * gvir_sandbox_config_interactive_set_command:
 * @config: (transfer none): the sandbox config
 * @argv: (transfer none)(array zero-terminated=1): the command path and arguments
 *
 * Set the path of the command to be run and its arguments. The @argv should
 * be a NULL terminated list
 */
void gvir_sandbox_config_interactive_set_command(GVirSandboxConfigInteractive *config, gchar **argv)
{
    GVirSandboxConfigInteractivePrivate *priv = config->priv;
    guint len = g_strv_length(argv);
    size_t i;
    g_strfreev(priv->command);
    priv->command = g_new0(gchar *, len + 1);
    for (i = 0 ; i < len ; i++)
        priv->command[i] = g_strdup(argv[i]);
    priv->command[len] = NULL;
}


static gchar **gvir_sandbox_config_interactive_get_command(GVirSandboxConfig *config)
{
    GVirSandboxConfigInteractive *iconfig = GVIR_SANDBOX_CONFIG_INTERACTIVE(config);
    GVirSandboxConfigInteractivePrivate *priv = iconfig->priv;
    return g_strdupv(priv->command);
}
