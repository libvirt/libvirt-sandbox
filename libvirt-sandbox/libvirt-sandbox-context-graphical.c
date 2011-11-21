/*
 * libvirt-sandbox-context-graphical.c: libvirt sandbox contexturation
 *
 * Copyright (C) 2011 Red Hat
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 *
 * Author: Daniel P. Berrange <berrange@redhat.com>
 */

#include <config.h>
#include <string.h>

#include "libvirt-sandbox/libvirt-sandbox.h"

#define GVIR_SANDBOX_CONTEXT_GRAPHICAL_GET_PRIVATE(obj)                         \
        (G_TYPE_INSTANCE_GET_PRIVATE((obj), GVIR_SANDBOX_TYPE_CONTEXT_GRAPHICAL, GVirSandboxContextGraphicalPrivate))

struct _GVirSandboxContextGraphicalPrivate
{
    gboolean unused;
};

G_DEFINE_TYPE(GVirSandboxContextGraphical, gvir_sandbox_context_graphical, GVIR_SANDBOX_TYPE_CONTEXT);


enum {
    PROP_0,
};

enum {
    LAST_SIGNAL
};

//static gint signals[LAST_SIGNAL];


static void gvir_sandbox_context_graphical_get_property(GObject *object,
                                                        guint prop_id,
                                                        GValue *value G_GNUC_UNUSED,
                                                        GParamSpec *pspec)
{
#if 0
    GVirSandboxContextGraphical *context = GVIR_SANDBOX_CONTEXT_GRAPHICAL(object);
    GVirSandboxContextGraphicalPrivate *priv = context->priv;
#endif

    switch (prop_id) {

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_context_graphical_set_property(GObject *object,
                                                        guint prop_id,
                                                        const GValue *value G_GNUC_UNUSED,
                                                        GParamSpec *pspec)
{
#if 0
    GVirSandboxContextGraphical *context = GVIR_SANDBOX_CONTEXT_GRAPHICAL(object);
    GVirSandboxContextGraphicalPrivate *priv = context->priv;
#endif

    switch (prop_id) {

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_context_graphical_finalize(GObject *object)
{
#if 0
    GVirSandboxContextGraphical *context = GVIR_SANDBOX_CONTEXT_GRAPHICAL(object);
    GVirSandboxContextGraphicalPrivate *priv = context->priv;
#endif

    G_OBJECT_CLASS(gvir_sandbox_context_graphical_parent_class)->finalize(object);
}


static void gvir_sandbox_context_graphical_class_init(GVirSandboxContextGraphicalClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = gvir_sandbox_context_graphical_finalize;
    object_class->get_property = gvir_sandbox_context_graphical_get_property;
    object_class->set_property = gvir_sandbox_context_graphical_set_property;

    g_type_class_add_private(klass, sizeof(GVirSandboxContextGraphicalPrivate));
}


static void gvir_sandbox_context_graphical_init(GVirSandboxContextGraphical *context)
{
    context->priv = GVIR_SANDBOX_CONTEXT_GRAPHICAL_GET_PRIVATE(context);
}


/**
 * gvir_sandbox_context_graphical_new:
 * @connection: (transfer none): the libvirt connection
 * @config: (transfer none): the initial configuratoion
 *
 * Create a new graphical application sandbox context
 *
 * Returns: (transfer full): a new graphical sandbox context object
 */
GVirSandboxContextGraphical *gvir_sandbox_context_graphical_new(GVirConnection *connection,
                                                                GVirSandboxConfig *config)
{
    return GVIR_SANDBOX_CONTEXT_GRAPHICAL(g_object_new(GVIR_SANDBOX_TYPE_CONTEXT_GRAPHICAL,
                                                       "connection", connection,
                                                       "config", config,
                                                       NULL));
}
