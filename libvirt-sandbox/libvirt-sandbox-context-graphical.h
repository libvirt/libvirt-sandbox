/*
 * libvirt-sandbox-context-graphical.h: libvirt sandbox contexturation
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

#ifndef __LIBVIRT_SANDBOX_CONTEXT_GRAPHICAL_H__
#define __LIBVIRT_SANDBOX_CONTEXT_GRAPHICAL_H__

G_BEGIN_DECLS

#define GVIR_SANDBOX_TYPE_CONTEXT_GRAPHICAL            (gvir_sandbox_context_graphical_get_type ())
#define GVIR_SANDBOX_CONTEXT_GRAPHICAL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GVIR_SANDBOX_TYPE_CONTEXT_GRAPHICAL, GVirSandboxContextGraphical))
#define GVIR_SANDBOX_CONTEXT_GRAPHICAL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GVIR_SANDBOX_TYPE_CONTEXT_GRAPHICAL, GVirSandboxContextGraphicalClass))
#define GVIR_SANDBOX_IS_CONTEXT_GRAPHICAL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GVIR_SANDBOX_TYPE_CONTEXT_GRAPHICAL))
#define GVIR_SANDBOX_IS_CONTEXT_GRAPHICAL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GVIR_SANDBOX_TYPE_CONTEXT_GRAPHICAL))
#define GVIR_SANDBOX_CONTEXT_GRAPHICAL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GVIR_SANDBOX_TYPE_CONTEXT_GRAPHICAL, GVirSandboxContextGraphicalClass))

typedef struct _GVirSandboxContextGraphical GVirSandboxContextGraphical;
typedef struct _GVirSandboxContextGraphicalPrivate GVirSandboxContextGraphicalPrivate;
typedef struct _GVirSandboxContextGraphicalClass GVirSandboxContextGraphicalClass;

struct _GVirSandboxContextGraphical
{
    GVirSandboxContext parent;

    GVirSandboxContextGraphicalPrivate *priv;

    /* Do not add fields to this struct */
};

struct _GVirSandboxContextGraphicalClass
{
    GVirSandboxContextClass parent_class;

};

GType gvir_sandbox_context_graphical_get_type(void);

GVirSandboxContextGraphical *gvir_sandbox_context_graphical_new(GVirConnection *connection,
                                                                GVirSandboxConfig *config);

#if 0
GVirSandboxGraphics *gvir_sandbox_context_get_graphics(GVirSandboxContext *ctxt);
#endif

G_END_DECLS

#endif /* __LIBVIRT_SANDBOX_CONTEXT_GRAPHICAL_H__ */
