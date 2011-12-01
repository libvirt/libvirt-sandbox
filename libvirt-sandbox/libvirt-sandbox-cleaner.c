/*
 * libvirt-sandbox-cleaner.c: libvirt sandbox cleaner
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 *
 * Author: Daniel P. Berrange <berrange@redhat.com>
 */

#include <config.h>

#include "libvirt-sandbox/libvirt-sandbox.h"

/**
 * SECTION: libvirt-sandbox-cleaner
 * @short_description: Sandbox context cleanup tasks
 * @include: libvirt-sandbox/libvirt-sandbox.h
 *
 * Provides an object for managing cleanup tasks associated with a sandbox
 *
 * The GVirSandboxCleaner object provides a framework for registering
 * cleanup tasks to be performed at various stages of a sandbox's
 * lifecycle. This is typically used to delete temporary files and
 * other similar state.
 */

#define GVIR_SANDBOX_CLEANER_GET_PRIVATE(obj)                         \
        (G_TYPE_INSTANCE_GET_PRIVATE((obj), GVIR_SANDBOX_TYPE_CLEANER, GVirSandboxCleanerPrivate))

struct _GVirSandboxCleanerPrivate
{
    GList *postStart;
    GList *postStop;
};

typedef struct _GVirSandboxCleanerEntry GVirSandboxCleanerEntry;
struct _GVirSandboxCleanerEntry
{
    GVirSandboxCleanerFunc func;
    gpointer opaque;
    GDestroyNotify ff;
};


G_DEFINE_TYPE(GVirSandboxCleaner, gvir_sandbox_cleaner, G_TYPE_OBJECT);


enum {
    PROP_0,

};

enum {
    LAST_SIGNAL
};

//static gint signals[LAST_SIGNAL];


static void gvir_sandbox_cleaner_get_property(GObject *object,
                                              guint prop_id,
                                              GValue *value G_GNUC_UNUSED,
                                              GParamSpec *pspec)
{
#if 0
    GVirSandboxCleaner *cleaner = GVIR_SANDBOX_CLEANER(object);
    GVirSandboxCleanerPrivate *priv = cleaner->priv;
#endif

    switch (prop_id) {

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_cleaner_set_property(GObject *object,
                                             guint prop_id,
                                             const GValue *value G_GNUC_UNUSED,
                                             GParamSpec *pspec)
{
#if 0
    GVirSandboxCleaner *cleaner = GVIR_SANDBOX_CLEANER(object);
    GVirSandboxCleanerPrivate *priv = cleaner->priv;
#endif

    switch (prop_id) {

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_cleaner_finalize(GObject *object)
{
    GVirSandboxCleaner *cleaner = GVIR_SANDBOX_CLEANER(object);
    GVirSandboxCleanerPrivate *priv = cleaner->priv;
    GList *tmp;

    tmp = priv->postStart;
    while (tmp) {
        GVirSandboxCleanerEntry *ent = tmp->data;
        ent->ff(ent->opaque);
        g_free(ent);
        tmp = tmp->next;
    }
    g_list_free(priv->postStart);

    tmp = priv->postStop;
    while (tmp) {
        GVirSandboxCleanerEntry *ent = tmp->data;
        ent->ff(ent->opaque);
        g_free(ent);
        tmp = tmp->next;
    }
    g_list_free(priv->postStop);

    G_OBJECT_CLASS(gvir_sandbox_cleaner_parent_class)->finalize(object);
}


static void gvir_sandbox_cleaner_class_init(GVirSandboxCleanerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = gvir_sandbox_cleaner_finalize;
    object_class->get_property = gvir_sandbox_cleaner_get_property;
    object_class->set_property = gvir_sandbox_cleaner_set_property;

    g_type_class_add_private(klass, sizeof(GVirSandboxCleanerPrivate));
}


static void gvir_sandbox_cleaner_init(GVirSandboxCleaner *cleaner)
{
    cleaner->priv = GVIR_SANDBOX_CLEANER_GET_PRIVATE(cleaner);
}


/**
 * gvir_sandbox_cleaner_new:
 *
 * Create a new sandbox cleaner
 *
 * Returns: (transfer full): a new sandbox cleaner object
 */
GVirSandboxCleaner *gvir_sandbox_cleaner_new(void)
{
    return GVIR_SANDBOX_CLEANER(g_object_new(GVIR_SANDBOX_TYPE_CLEANER,
                                             NULL));
}


void gvir_sandbox_cleaner_add_action_post_start(GVirSandboxCleaner *cleaner,
                                                GVirSandboxCleanerFunc func,
                                                gpointer opaque,
                                                GDestroyNotify ff)
{
    GVirSandboxCleanerPrivate *priv = cleaner->priv;
    GVirSandboxCleanerEntry *ent = g_new0(GVirSandboxCleanerEntry, 1);

    ent->func = func;
    ent->opaque = opaque;
    ent->ff = ff;

    priv->postStart = g_list_append(priv->postStart, ent);
}


void gvir_sandbox_cleaner_add_action_post_stop(GVirSandboxCleaner *cleaner,
                                               GVirSandboxCleanerFunc func,
                                               gpointer opaque,
                                               GDestroyNotify ff)
{
    GVirSandboxCleanerPrivate *priv = cleaner->priv;
    GVirSandboxCleanerEntry *ent = g_new0(GVirSandboxCleanerEntry, 1);

    ent->func = func;
    ent->opaque = opaque;
    ent->ff = ff;

    priv->postStop = g_list_append(priv->postStop, ent);
}


gboolean gvir_sandbox_cleaner_run_post_start(GVirSandboxCleaner *cleaner, GError **error)
{
    GVirSandboxCleanerPrivate *priv = cleaner->priv;
    GList *tmp = priv->postStart;
    gboolean ret = TRUE;

    while (tmp) {
        GVirSandboxCleanerEntry *ent = tmp->data;

        if (!ent->func(cleaner, error, ent->opaque))
            ret = FALSE;

        tmp = tmp->next;
    }

    return ret;
}


gboolean gvir_sandbox_cleaner_run_post_stop(GVirSandboxCleaner *cleaner, GError **error)
{
    GVirSandboxCleanerPrivate *priv = cleaner->priv;
    GList *tmp = priv->postStop;
    gboolean ret = TRUE;

    while (tmp) {
        GVirSandboxCleanerEntry *ent = tmp->data;

        if (!ent->func(cleaner, error, ent->opaque))
            ret = FALSE;

        tmp = tmp->next;
    }

    return ret;
}
