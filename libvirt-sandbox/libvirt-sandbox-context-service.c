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

static gboolean gvir_sandbox_context_service_prestart(GVirSandboxContext *ctxt,
                                                      const gchar *configdir,
                                                      GError **error);

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
    GVirSandboxContextClass *context_class = GVIR_SANDBOX_CONTEXT_CLASS(klass);

    object_class->finalize = gvir_sandbox_context_service_finalize;
    object_class->get_property = gvir_sandbox_context_service_get_property;
    object_class->set_property = gvir_sandbox_context_service_set_property;

    context_class->prestart = gvir_sandbox_context_service_prestart;

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

static gboolean gvir_sandbox_context_service_write_target(GVirSandboxContext *ctxt,
                                                          const gchar *unitfile,
                                                          GError **error)
{
    gboolean ret = FALSE;
    GFile *file = g_file_new_for_path(unitfile);
    const gchar *unitdata =
        "[Unit]\n"
        "Description=Sandbox Target\n"
        "After=multi-user.target\n";

   if (!g_file_replace_contents(file, unitdata, strlen(unitdata),
                                NULL, FALSE,
                                G_FILE_CREATE_NONE,
                                NULL,
                                NULL,
                                error))
       goto cleanup;

   gvir_sandbox_cleaner_add_rmfile_post_stop(gvir_sandbox_context_get_cleaner(ctxt),
                                             unitfile);

   ret = TRUE;

cleanup:
   g_object_unref(file);
   return ret;
}


static gboolean gvir_sandbox_context_service_write_target_deps(GVirSandboxContext *ctxt,
                                                               const gchar *unitdir,
                                                               GError **error)
{
    gboolean ret = FALSE;
    GList *units = NULL, *tmp;
    GVirSandboxConfigService *cfg = GVIR_SANDBOX_CONFIG_SERVICE(
        gvir_sandbox_context_get_config(ctxt));

    units = tmp = gvir_sandbox_config_service_get_units(cfg);
    while (tmp) {
        const gchar *unitname = tmp->data;
        gchar *unittgt = g_build_filename(unitdir, unitname, NULL);
        gchar *unitsrc = g_build_filename("/", "lib", "systemd", "system", unitname, NULL);
        GFile *file = g_file_new_for_path(unittgt);

        gboolean rv = g_file_make_symbolic_link(file, unitsrc, NULL, error);
        g_free(unitsrc);
        g_object_unref(file);
        if (!rv) {
            g_free(unittgt);
            goto cleanup;
        }

        gvir_sandbox_cleaner_add_rmfile_post_stop(gvir_sandbox_context_get_cleaner(ctxt),
                                                  unittgt);

        tmp = tmp->next;
    }

    ret = TRUE;

cleanup:
    g_list_free(units);
    return ret;
}


static gboolean gvir_sandbox_context_service_prestart(GVirSandboxContext *ctxt,
                                                      const gchar *configdir,
                                                      GError **error)
{
    gchar *unitdir = NULL;
    gchar *tgtdir = NULL;
    gchar *tgtfile = NULL;
    gboolean ret = FALSE;

    if (!(GVIR_SANDBOX_CONTEXT_CLASS(gvir_sandbox_context_service_parent_class)->prestart(ctxt, configdir, error)))
        return FALSE;

    unitdir = g_build_filename(configdir, "systemd", NULL);
    tgtfile = g_build_filename(unitdir, "sandbox.target", NULL);
    tgtdir = g_build_filename(unitdir, "sandbox.target.wants", NULL);

    g_mkdir_with_parents(tgtdir, 0700);
    gvir_sandbox_cleaner_add_rmdir_post_stop(gvir_sandbox_context_get_cleaner(ctxt),
                                             unitdir);
    gvir_sandbox_cleaner_add_rmdir_post_stop(gvir_sandbox_context_get_cleaner(ctxt),
                                             tgtdir);

    if (!gvir_sandbox_context_service_write_target(ctxt, tgtfile, error))
        goto cleanup;
    if (!gvir_sandbox_context_service_write_target_deps(ctxt, tgtdir, error))
        goto cleanup;

    ret = TRUE;
cleanup:
    g_free(tgtdir);
    g_free(unitdir);
    return ret;
}
