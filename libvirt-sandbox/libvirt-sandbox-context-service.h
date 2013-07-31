/*
 * libvirt-sandbox-context-service.h: libvirt sandbox service context
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

#if !defined(__LIBVIRT_SANDBOX_H__) && !defined(LIBVIRT_SANDBOX_BUILD)
#error "Only <libvirt-sandbox/libvirt-sandbox.h> can be included directly."
#endif

#ifndef __LIBVIRT_SANDBOX_CONTEXT_SERVICE_H__
#define __LIBVIRT_SANDBOX_CONTEXT_SERVICE_H__

G_BEGIN_DECLS

#define GVIR_SANDBOX_TYPE_CONTEXT_SERVICE            (gvir_sandbox_context_service_get_type ())
#define GVIR_SANDBOX_CONTEXT_SERVICE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GVIR_SANDBOX_TYPE_CONTEXT_SERVICE, GVirSandboxContextService))
#define GVIR_SANDBOX_CONTEXT_SERVICE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GVIR_SANDBOX_TYPE_CONTEXT_SERVICE, GVirSandboxContextServiceClass))
#define GVIR_SANDBOX_IS_CONTEXT_SERVICE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GVIR_SANDBOX_TYPE_CONTEXT_SERVICE))
#define GVIR_SANDBOX_IS_CONTEXT_SERVICE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GVIR_SANDBOX_TYPE_CONTEXT_SERVICE))
#define GVIR_SANDBOX_CONTEXT_SERVICE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GVIR_SANDBOX_TYPE_CONTEXT_SERVICE, GVirSandboxContextServiceClass))

typedef struct _GVirSandboxContextService GVirSandboxContextService;
typedef struct _GVirSandboxContextServicePrivate GVirSandboxContextServicePrivate;
typedef struct _GVirSandboxContextServiceClass GVirSandboxContextServiceClass;

struct _GVirSandboxContextService
{
    GVirSandboxContext parent;

    GVirSandboxContextServicePrivate *priv;

    /* Do not add fields to this struct */
};

struct _GVirSandboxContextServiceClass
{
    GVirSandboxContextClass parent_class;

    gboolean (*define)(GVirSandboxContextService *ctxt, GError **error);
    gboolean (*undefine)(GVirSandboxContextService *ctxt, GError **error);

    gpointer padding[LIBVIRT_SANDBOX_CLASS_PADDING];
};

GType gvir_sandbox_context_service_get_type(void);

GVirSandboxContextService *gvir_sandbox_context_service_new(GVirConnection *connection,
                                                            GVirSandboxConfigService *config);

gboolean gvir_sandbox_context_service_define(GVirSandboxContextService *ctxt, GError **error);
gboolean gvir_sandbox_context_service_undefine(GVirSandboxContextService *ctxt, GError **error);

G_END_DECLS

#endif /* __LIBVIRT_SANDBOX_CONTEXT_SERVICE_H__ */
