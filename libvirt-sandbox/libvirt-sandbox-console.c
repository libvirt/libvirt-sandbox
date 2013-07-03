/*
 * libvirt-sandbox-console.c: libvirt sandbox console
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

#include <glib/gi18n.h>

#include "libvirt-sandbox/libvirt-sandbox.h"

/**
 * SECTION: libvirt-sandbox-console
 * @short_description: A text mode console
 * @include: libvirt-sandbox/libvirt-sandbox.h
 *
 * Provides an object to interface to the text mode console of the sandbox
 *
 * The GVirSandboxConsole object provides support for interfacing to the
 * text mode console of the sandbox. It forwards I/O between the #GVirStream
 * associated with the virtual machine's console and a local console
 * represented by #GUnixInputStream and #GUnixOutputStream objects.
 *
 */

#define GVIR_SANDBOX_CONSOLE_GET_PRIVATE(obj)                       \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), GVIR_SANDBOX_TYPE_CONSOLE, GVirSandboxConsolePrivate))


struct _GVirSandboxConsolePrivate
{
    GVirConnection *connection;
    GVirDomain *domain;
    gchar *devname;
    gchar escape;
    gboolean direct;
};


G_DEFINE_ABSTRACT_TYPE(GVirSandboxConsole, gvir_sandbox_console, G_TYPE_OBJECT);

enum {
    PROP_0,

    PROP_CONNECTION,
    PROP_DOMAIN,
    PROP_DEVNAME,
    PROP_ESCAPE,
    PROP_DIRECT,
};

enum {
    LAST_SIGNAL
};

//static gint signals[LAST_SIGNAL];

#define GVIR_SANDBOX_CONSOLE_ERROR gvir_sandbox_console_error_quark()

static GQuark
gvir_sandbox_console_error_quark(void)
{
    return g_quark_from_static_string("gvir-sandbox-console");
}


static void gvir_sandbox_console_get_property(GObject *object,
                                              guint prop_id,
                                              GValue *value,
                                              GParamSpec *pspec)
{
    GVirSandboxConsole *console = GVIR_SANDBOX_CONSOLE(object);
    GVirSandboxConsolePrivate *priv = console->priv;

    switch (prop_id) {
    case PROP_CONNECTION:
        g_value_set_object(value, priv->connection);
        break;

    case PROP_DOMAIN:
        g_value_set_object(value, priv->domain);
        break;

    case PROP_DEVNAME:
        g_value_set_string(value, priv->devname);
        break;

    case PROP_ESCAPE:
        g_value_set_schar(value, priv->escape);
        break;

    case PROP_DIRECT:
        g_value_set_boolean(value, priv->direct);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_console_set_property(GObject *object,
                                              guint prop_id,
                                              const GValue *value,
                                              GParamSpec *pspec)
{
    GVirSandboxConsole *console = GVIR_SANDBOX_CONSOLE(object);
    GVirSandboxConsolePrivate *priv = console->priv;

    switch (prop_id) {
    case PROP_CONNECTION:
        if (priv->connection)
            g_object_unref(priv->connection);
        priv->connection = g_value_dup_object(value);
        break;

    case PROP_DOMAIN:
        if (priv->domain)
            g_object_unref(priv->domain);
        priv->domain = g_value_dup_object(value);
        break;

    case PROP_DEVNAME:
        priv->devname = g_value_dup_string(value);
        break;

    case PROP_ESCAPE:
        priv->escape = g_value_get_schar(value);
        break;

    case PROP_DIRECT:
        priv->direct = g_value_get_boolean(value);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_console_finalize(GObject *object)
{
    GVirSandboxConsole *console = GVIR_SANDBOX_CONSOLE(object);
    GVirSandboxConsolePrivate *priv = console->priv;

    if (priv->domain)
        g_object_unref(priv->domain);
    if (priv->connection)
        g_object_unref(priv->connection);

    g_free(priv->devname);

    G_OBJECT_CLASS(gvir_sandbox_console_parent_class)->finalize(object);
}


static void gvir_sandbox_console_class_init(GVirSandboxConsoleClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = gvir_sandbox_console_finalize;
    object_class->get_property = gvir_sandbox_console_get_property;
    object_class->set_property = gvir_sandbox_console_set_property;

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
    g_object_class_install_property(object_class,
                                    PROP_DOMAIN,
                                    g_param_spec_object("domain",
                                                        "Domain",
                                                        "The sandbox domain",
                                                        GVIR_TYPE_DOMAIN,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_DEVNAME,
                                    g_param_spec_string("devname",
                                                        "Devicename",
                                                        "Device name",
                                                        NULL,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_ESCAPE,
                                    g_param_spec_char("escape",
                                                      "Escape",
                                                      "Escape character",
                                                      0,
                                                      127,
                                                      ']',
                                                      G_PARAM_READABLE |
                                                      G_PARAM_WRITABLE |
                                                      G_PARAM_CONSTRUCT_ONLY |
                                                      G_PARAM_STATIC_NAME |
                                                      G_PARAM_STATIC_NICK |
                                                      G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_DIRECT,
                                    g_param_spec_boolean("direct",
                                                         "Direct",
                                                         "Direct pty access",
                                                         FALSE,
                                                         G_PARAM_READABLE |
                                                         G_PARAM_WRITABLE |
                                                         G_PARAM_CONSTRUCT_ONLY |
                                                         G_PARAM_STATIC_NAME |
                                                         G_PARAM_STATIC_NICK |
                                                         G_PARAM_STATIC_BLURB));

    g_signal_new("closed",
                 G_OBJECT_CLASS_TYPE(object_class),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(GVirSandboxConsoleClass, closed),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__BOOLEAN,
                 G_TYPE_NONE,
                 1,
                 G_TYPE_BOOLEAN);

    g_type_class_add_private(klass, sizeof(GVirSandboxConsolePrivate));
}


static void gvir_sandbox_console_init(GVirSandboxConsole *console)
{
    console->priv = GVIR_SANDBOX_CONSOLE_GET_PRIVATE(console);
}


gboolean gvir_sandbox_console_attach(GVirSandboxConsole *console,
                                     GUnixInputStream *localStdin,
                                     GUnixOutputStream *localStdout,
                                     GUnixOutputStream *localStderr,
                                     GError **error)
{
    return GVIR_SANDBOX_CONSOLE_GET_CLASS(console)->attach(console, localStdin, localStdout, localStderr, error);
}

gboolean gvir_sandbox_console_attach_stdio(GVirSandboxConsole *console_,
                                           GError **error)
{
    GInputStream *localStdin = g_unix_input_stream_new(STDIN_FILENO, FALSE);
    GOutputStream *localStdout = g_unix_output_stream_new(STDOUT_FILENO, FALSE);
    GOutputStream *localStderr = g_unix_output_stream_new(STDERR_FILENO, FALSE);
    gboolean ret;

    ret = gvir_sandbox_console_attach(console_,
                                      G_UNIX_INPUT_STREAM(localStdin),
                                      G_UNIX_OUTPUT_STREAM(localStdout),
                                      G_UNIX_OUTPUT_STREAM(localStderr),
                                      error);

    g_object_unref(localStdin);
    g_object_unref(localStdout);
    g_object_unref(localStderr);

    return ret;
}


gboolean gvir_sandbox_console_attach_stderr(GVirSandboxConsole *console_,
                                           GError **error)
{
    GOutputStream *localStderr = g_unix_output_stream_new(STDERR_FILENO, FALSE);
    gboolean ret;

    ret = gvir_sandbox_console_attach(console_,
                                      NULL, NULL,
                                      G_UNIX_OUTPUT_STREAM(localStderr),
                                      error);

    g_object_unref(localStderr);

    return ret;
}


gboolean gvir_sandbox_console_detach(GVirSandboxConsole *console,
                                     GError **error)
{
    return GVIR_SANDBOX_CONSOLE_GET_CLASS(console)->detach(console, error);
}

gboolean gvir_sandbox_console_isolate(GVirSandboxConsole *console,
                                      GError **error)
{
    GVirSandboxConsolePrivate *priv = console->priv;

    if (!console->priv->direct) {
        g_set_error(error, GVIR_SANDBOX_CONSOLE_ERROR, 0, "%s",
                    _("Unable to isolate console unless direct access flag is set"));
        return FALSE;
    }

    if (priv->connection) {
        g_object_unref(priv->connection);
        priv->connection = NULL;
    }
    if (priv->domain) {
        g_object_unref(priv->domain);
        priv->domain = NULL;
    }

    return TRUE;
}


void gvir_sandbox_console_set_escape(GVirSandboxConsole *console,
                                     gchar escape)
{
    console->priv->escape = escape;
}


gchar gvir_sandbox_console_get_escape(GVirSandboxConsole *console)
{
    return console->priv->escape;
}


void gvir_sandbox_console_set_direct(GVirSandboxConsole *console,
                                     gboolean direct)
{
    console->priv->direct = direct;
}


gboolean gvir_sandbox_console_get_direct(GVirSandboxConsole *console)
{
    return console->priv->direct;
}
