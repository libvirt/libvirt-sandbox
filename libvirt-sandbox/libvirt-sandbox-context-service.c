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


static gchar *gvir_sandbox_context_service_get_unitdata(GVirSandboxContext *ctxt)
{
    GVirSandboxConfig *config = gvir_sandbox_context_get_config(ctxt);
    GVirSandboxConfigService *sconfig = GVIR_SANDBOX_CONFIG_SERVICE(config);
    gchar *ret = NULL;

    /* XXX forking should not be hardcoded */
    /* XXX capabilities */
    ret = g_strdup_printf(
        "[Unit]\n"
        "Description=Sandbox Service\n"
        "DefaultDependencies=no\n"
        "\n"
        "[Service]\n"
        "Type=forking\n"
        "User=%d\n"
        "Group=%d\n"
        "StandardInput=null\n"
        "StandardOutput=null\n"
        "StandardError=null\n"
        "WorkingDirectory=%s\n"
        "Environment=HOME=%s USER=%s\n"
        "ExecStart=%s\n"
        "ExecStopPost=/bin/systemctl --force poweroff\n",
        gvir_sandbox_config_get_userid(config),
        gvir_sandbox_config_get_groupid(config),
        gvir_sandbox_config_get_homedir(config),
        gvir_sandbox_config_get_homedir(config),
        gvir_sandbox_config_get_username(config),
        gvir_sandbox_config_service_get_executable(sconfig));

    return ret;
}

static gboolean gvir_sandbox_context_service_write_unitfile(GVirSandboxContext *ctxt,
                                                            const gchar *unitfile,
                                                            GError **error)
{
    gboolean ret = FALSE;
    GFile *file = g_file_new_for_path(unitfile);
    gchar *unitdata;

    unitdata = gvir_sandbox_context_service_get_unitdata(ctxt);

    if (!g_file_replace_contents(file, unitdata, strlen(unitdata),
                                 NULL, FALSE,
                                 G_FILE_CREATE_NONE,
                                 NULL,
                                 NULL,
                                 error))
        goto cleanup;

    ret = TRUE;

cleanup:
    g_object_unref(file);
    g_free(unitdata);
    return ret;
}


static gboolean gvir_sandbox_context_service_prestart(GVirSandboxContext *ctxt,
                                                      const gchar *configdir,
                                                      GError **error)
{
    gchar *unitdir = NULL;
    gchar *unitfile = NULL;
    gboolean ret = FALSE;

    if (!(GVIR_SANDBOX_CONTEXT_CLASS(gvir_sandbox_context_service_parent_class)->prestart(ctxt, configdir, error)))
        return FALSE;

    unitdir = g_build_filename(configdir, "systemd", NULL);
    unitfile = g_build_filename(unitdir, "sandbox.service", NULL);

    g_mkdir_with_parents(unitdir, 0700);
    gvir_sandbox_cleaner_add_rmdir_post_stop(gvir_sandbox_context_get_cleaner(ctxt),
                                             unitdir);

    if (!gvir_sandbox_context_service_write_unitfile(ctxt, unitfile, error))
        goto cleanup;

    gvir_sandbox_cleaner_add_rmfile_post_stop(gvir_sandbox_context_get_cleaner(ctxt),
                                              unitfile);

    ret = TRUE;
cleanup:
    g_free(unitfile);
    g_free(unitdir);
    return ret;
}
