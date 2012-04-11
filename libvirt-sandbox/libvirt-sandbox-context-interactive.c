/*
 * libvirt-sandbox-context-interactive.c: libvirt sandbox interactive context
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
 * SECTION: libvirt-sandbox-context-interactive
 * @short_description: Desktop application sandbox context
 * @include: libvirt-sandbox/libvirt-sandbox.h
 * @see_also: #GVirSandboxContext
 *
 * Provides a base class for implementing interactive desktop application sandboxes
 *
 * The GVirSandboxContextInteractive object extends the functionality provided by
 * #GVirSandboxContext to allow the application to display output in a interactive
 * desktop.
 */

#define GVIR_SANDBOX_CONTEXT_INTERACTIVE_GET_PRIVATE(obj)                         \
        (G_TYPE_INSTANCE_GET_PRIVATE((obj), GVIR_SANDBOX_TYPE_CONTEXT_INTERACTIVE, GVirSandboxContextInteractivePrivate))

struct _GVirSandboxContextInteractivePrivate
{
    gboolean unused;
};

G_DEFINE_TYPE(GVirSandboxContextInteractive, gvir_sandbox_context_interactive, GVIR_SANDBOX_TYPE_CONTEXT);


enum {
    PROP_0,
};

enum {
    LAST_SIGNAL
};

//static gint signals[LAST_SIGNAL];


static void gvir_sandbox_context_interactive_get_property(GObject *object,
                                                          guint prop_id,
                                                          GValue *value G_GNUC_UNUSED,
                                                          GParamSpec *pspec)
{
#if 0
    GVirSandboxContextInteractive *context = GVIR_SANDBOX_CONTEXT_INTERACTIVE(object);
    GVirSandboxContextInteractivePrivate *priv = context->priv;
#endif

    switch (prop_id) {

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_context_interactive_set_property(GObject *object,
                                                          guint prop_id,
                                                          const GValue *value G_GNUC_UNUSED,
                                                          GParamSpec *pspec)
{
#if 0
    GVirSandboxContextInteractive *context = GVIR_SANDBOX_CONTEXT_INTERACTIVE(object);
    GVirSandboxContextInteractivePrivate *priv = context->priv;
#endif

    switch (prop_id) {

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_context_interactive_finalize(GObject *object)
{
#if 0
    GVirSandboxContextInteractive *context = GVIR_SANDBOX_CONTEXT_INTERACTIVE(object);
    GVirSandboxContextInteractivePrivate *priv = context->priv;
#endif

    G_OBJECT_CLASS(gvir_sandbox_context_interactive_parent_class)->finalize(object);
}


static void gvir_sandbox_context_interactive_class_init(GVirSandboxContextInteractiveClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = gvir_sandbox_context_interactive_finalize;
    object_class->get_property = gvir_sandbox_context_interactive_get_property;
    object_class->set_property = gvir_sandbox_context_interactive_set_property;

    g_type_class_add_private(klass, sizeof(GVirSandboxContextInteractivePrivate));
}


static void gvir_sandbox_context_interactive_init(GVirSandboxContextInteractive *context)
{
    context->priv = GVIR_SANDBOX_CONTEXT_INTERACTIVE_GET_PRIVATE(context);
}


/**
 * gvir_sandbox_context_interactive_new:
 * @connection: (transfer none): the libvirt connection
 * @config: (transfer none): the initial configuratoion
 *
 * Create a new interactive application sandbox context
 *
 * Returns: (transfer full): a new interactive sandbox context object
 */
GVirSandboxContextInteractive *gvir_sandbox_context_interactive_new(GVirConnection *connection,
                                                                    GVirSandboxConfigInteractive *config)
{
    return GVIR_SANDBOX_CONTEXT_INTERACTIVE(g_object_new(GVIR_SANDBOX_TYPE_CONTEXT_INTERACTIVE,
                                                         "connection", connection,
                                                         "config", config,
                                                         NULL));
}


/**
 * gvir_sandbox_context_interactive_get_app_console:
 * @ctxt: (transfer none): the sandbox context
 *
 * Returns: (transfer full)(allow-none): the sandbox console (or NULL)
 */
GVirSandboxConsole *gvir_sandbox_context_interactive_get_app_console(GVirSandboxContextInteractive *ctxt,
                                                                     GError **error)
{
    GVirSandboxConsole *console;
    const char *devname = NULL;
    GVirDomain *domain;
    GVirConnection *conn;

    if (!(domain = gvir_sandbox_context_get_domain(GVIR_SANDBOX_CONTEXT(ctxt), error)))
        return NULL;

    conn = gvir_sandbox_context_get_connection(GVIR_SANDBOX_CONTEXT(ctxt));

    /* XXX get from config + shell */
    if (strstr(gvir_connection_get_uri(conn), "lxc")) {
        GVirSandboxConfig *config = gvir_sandbox_context_get_config(GVIR_SANDBOX_CONTEXT(ctxt));
        if (gvir_sandbox_config_get_shell(config))
            devname = "console2";
        else
            devname = "console1";
    } else {
        devname = "console1";
    }

    console = GVIR_SANDBOX_CONSOLE(gvir_sandbox_console_rpc_new(conn,
                                                                domain,
                                                                devname));
    g_object_unref(domain);
    return console;
}
