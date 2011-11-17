/*
 * libvirt-sandbox-cleaner.h: libvirt sandbox cleaner
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

#if !defined(__LIBVIRT_SANDBOX_H__) && !defined(LIBVIRT_SANDBOX_BUILD)
#error "Only <libvirt-sandbox/libvirt-sandbox.h> can be included directly."
#endif

#ifndef __LIBVIRT_SANDBOX_CLEANER_H__
#define __LIBVIRT_SANDBOX_CLEANER_H__

G_BEGIN_DECLS

#define GVIR_SANDBOX_TYPE_CLEANER            (gvir_sandbox_cleaner_get_type ())
#define GVIR_SANDBOX_CLEANER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GVIR_SANDBOX_TYPE_CLEANER, GVirSandboxCleaner))
#define GVIR_SANDBOX_CLEANER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GVIR_SANDBOX_TYPE_CLEANER, GVirSandboxCleanerClass))
#define GVIR_SANDBOX_IS_CLEANER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GVIR_SANDBOX_TYPE_CLEANER))
#define GVIR_SANDBOX_IS_CLEANER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GVIR_SANDBOX_TYPE_CLEANER))
#define GVIR_SANDBOX_CLEANER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GVIR_SANDBOX_TYPE_CLEANER, GVirSandboxCleanerClass))

#define GVIR_SANDBOX_TYPE_CLEANER_HANDLE      (gvir_sandbox_cleaner_handle_get_type ())

typedef struct _GVirSandboxCleaner GVirSandboxCleaner;
typedef struct _GVirSandboxCleanerPrivate GVirSandboxCleanerPrivate;
typedef struct _GVirSandboxCleanerClass GVirSandboxCleanerClass;

struct _GVirSandboxCleaner
{
    GObject parent;

    GVirSandboxCleanerPrivate *priv;

    /* Do not add fields to this struct */
};

struct _GVirSandboxCleanerClass
{
    GObjectClass parent_class;

};

GType gvir_sandbox_cleaner_get_type(void);

GVirSandboxCleaner *gvir_sandbox_cleaner_new(void);

typedef gboolean (*GVirSandboxCleanerFunc)(GVirSandboxCleaner *ctxt,
                                           GError **error,
                                           gpointer opaque);

void gvir_sandbox_cleaner_add_action_post_start(GVirSandboxCleaner *ctxt,
                                                GVirSandboxCleanerFunc func,
                                                gpointer opaque,
                                                GDestroyNotify ff);

void gvir_sandbox_cleaner_add_action_post_stop(GVirSandboxCleaner *ctxt,
                                               GVirSandboxCleanerFunc func,
                                               gpointer opaque,
                                               GDestroyNotify ff);

gboolean gvir_sandbox_cleaner_run_post_start(GVirSandboxCleaner *ctxt, GError **error);
gboolean gvir_sandbox_cleaner_run_post_stop(GVirSandboxCleaner *ctxt, GError **error);

G_END_DECLS

#endif /* __LIBVIRT_SANDBOX_CLEANER_H__ */
