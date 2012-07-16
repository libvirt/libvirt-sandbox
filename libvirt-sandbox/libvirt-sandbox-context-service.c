/*
 * libvirt-sandbox-context-service.c: libvirt sandbox service context
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
 * SECTION: libvirt-sandbox-context-service
 * @short_description: Desktop application sandbox context
 * @include: libvirt-sandbox/libvirt-sandbox.h
 * @see_also: #GVirSandboxContext
 *
 * Provides a base class for implementing service desktop application sandboxes
 *
 * The GVirSandboxContextService object extends the functionality provided by
 * #GVirSandboxContext to allow the application to display output in a service
 * desktop.
 */

#define GVIR_SANDBOX_CONTEXT_SERVICE_GET_PRIVATE(obj)                         \
        (G_TYPE_INSTANCE_GET_PRIVATE((obj), GVIR_SANDBOX_TYPE_CONTEXT_SERVICE, GVirSandboxContextServicePrivate))

struct _GVirSandboxContextServicePrivate
{
    gboolean unused;
};

G_DEFINE_TYPE(GVirSandboxContextService, gvir_sandbox_context_service, GVIR_SANDBOX_TYPE_CONTEXT);


enum {
    PROP_0,
};

enum {
    LAST_SIGNAL
};

//static gint signals[LAST_SIGNAL];

static void gvir_sandbox_context_service_get_property(GObject *object,
                                                      guint prop_id,
                                                      GValue *value G_GNUC_UNUSED,
                                                      GParamSpec *pspec)
{
#if 0
    GVirSandboxContextService *context = GVIR_SANDBOX_CONTEXT_SERVICE(object);
    GVirSandboxContextServicePrivate *priv = context->priv;
#endif

    switch (prop_id) {

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_context_service_set_property(GObject *object,
                                                      guint prop_id,
                                                      const GValue *value G_GNUC_UNUSED,
                                                      GParamSpec *pspec)
{
#if 0
    GVirSandboxContextService *context = GVIR_SANDBOX_CONTEXT_SERVICE(object);
    GVirSandboxContextServicePrivate *priv = context->priv;
#endif

    switch (prop_id) {

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_context_service_finalize(GObject *object)
{
#if 0
    GVirSandboxContextService *context = GVIR_SANDBOX_CONTEXT_SERVICE(object);
    GVirSandboxContextServicePrivate *priv = context->priv;
#endif

    G_OBJECT_CLASS(gvir_sandbox_context_service_parent_class)->finalize(object);
}


static void gvir_sandbox_context_service_class_init(GVirSandboxContextServiceClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = gvir_sandbox_context_service_finalize;
    object_class->get_property = gvir_sandbox_context_service_get_property;
    object_class->set_property = gvir_sandbox_context_service_set_property;

    g_type_class_add_private(klass, sizeof(GVirSandboxContextServicePrivate));
}


static void gvir_sandbox_context_service_init(GVirSandboxContextService *context)
{
    context->priv = GVIR_SANDBOX_CONTEXT_SERVICE_GET_PRIVATE(context);
}


/**
 * gvir_sandbox_context_service_new:
 * @connection: (transfer none): the libvirt connection
 * @config: (transfer none): the initial configuratoion
 *
 * Create a new service application sandbox context
 *
 * Returns: (transfer full): a new service sandbox context object
 */
GVirSandboxContextService *gvir_sandbox_context_service_new(GVirConnection *connection,
                                                            GVirSandboxConfigService *config)
{
    return GVIR_SANDBOX_CONTEXT_SERVICE(g_object_new(GVIR_SANDBOX_TYPE_CONTEXT_SERVICE,
                                                     "connection", connection,
                                                     "config", config,
                                                     NULL));
}
