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
#include <errno.h>

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

#define GVIR_SANDBOX_CONTEXT_SERVICE_ERROR gvir_sandbox_context_service_error_quark()

static GQuark
gvir_sandbox_context_service_error_quark(void)
{
    return g_quark_from_static_string("gvir-sandbox-context-service");
}

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

static gboolean gvir_sandbox_context_service_define_default(GVirSandboxContextService *ctxt, GError **error)
{
    GVirConfigDomain *configdom = NULL;
    GVirSandboxBuilder *builder = NULL;
    GVirSandboxConfig *config;
    GVirConnection *connection;
    GVirDomain *domain = NULL;
    const gchar *sysconfdir;
    gchar *servicedir;
    gchar *statedir;
    gchar *sandboxdir;
    gchar *configdir;
    gchar *emptydir;
    gchar *configfile;
    gboolean ret = FALSE;

    connection = gvir_sandbox_context_get_connection(GVIR_SANDBOX_CONTEXT(ctxt));
    config = gvir_sandbox_context_get_config(GVIR_SANDBOX_CONTEXT(ctxt));

    sysconfdir = (getuid() ? g_get_user_config_dir() : SYSCONFDIR);
    sandboxdir = g_build_filename(sysconfdir, "libvirt-sandbox", NULL);
    servicedir = g_build_filename(sandboxdir, "services", NULL);
    statedir = g_build_filename(servicedir,
                                gvir_sandbox_config_get_name(config),
                                NULL);
    configdir = g_build_filename(statedir, "config", NULL);
    configfile = g_build_filename(configdir, "sandbox.cfg", NULL);
    emptydir = g_build_filename(configdir, "empty", NULL);

    if (!(builder = gvir_sandbox_builder_for_connection(connection,
                                                        error)))
        goto cleanup;

    g_mkdir_with_parents(sandboxdir, 0700);
    g_mkdir_with_parents(servicedir, 0700);
    g_mkdir_with_parents(statedir, 0700);
    g_mkdir_with_parents(configdir, 0700);

    if (!(configdom = gvir_sandbox_builder_construct(builder,
                                                     config,
                                                     statedir,
                                                     error))) {
        goto cleanup;
    }

    if (!(domain = gvir_connection_create_domain(connection,
                                                 configdom,
                                                 error))) {
        goto cleanup;
    }

    unlink(configfile);
    if (!gvir_sandbox_config_save_to_path(config, configfile, error))
        goto cleanup;

    g_mkdir_with_parents(emptydir, 0755);

    ret = TRUE;
cleanup:
    if (!ret)
        unlink(configfile);

    g_free(servicedir);
    g_free(sandboxdir);
    g_free(statedir);
    g_free(configdir);
    g_free(configfile);
    g_free(emptydir);
    if (domain)
        g_object_unref(domain);
    if (configdom)
        g_object_unref(configdom);
    if (builder)
        g_object_unref(builder);
    if (connection)
        g_object_unref(connection);
    if (config)
        g_object_unref(config);

    return ret;
}


static gboolean gvir_sandbox_context_service_undefine_default(GVirSandboxContextService *ctxt, GError **error)
{
    GVirSandboxBuilder *builder = NULL;
    GVirSandboxConfig *config;
    GVirConnection *connection;
    GVirDomain *domain = NULL;
    const gchar *sysconfdir;
    gchar *servicedir;
    gchar *sandboxdir;
    gchar *statedir;
    gchar *configdir;
    gchar *configfile;
    gchar *emptydir;
    gboolean ret = TRUE;

    connection = gvir_sandbox_context_get_connection(GVIR_SANDBOX_CONTEXT(ctxt));
    config = gvir_sandbox_context_get_config(GVIR_SANDBOX_CONTEXT(ctxt));

    sysconfdir = (getuid() ? g_get_user_config_dir() : SYSCONFDIR);
    sandboxdir = g_build_filename(sysconfdir, "libvirt-sandbox", NULL);
    servicedir = g_build_filename(sandboxdir, "services", NULL);
    statedir = g_build_filename(servicedir,
                                gvir_sandbox_config_get_name(config),
                                NULL);
    configdir = g_build_filename(statedir, "config", NULL);
    configfile = g_build_filename(configdir, "sandbox.cfg", NULL);
    emptydir = g_build_filename(configdir, "empty", NULL);

    if (!gvir_connection_fetch_domains(connection, NULL, error))
        goto cleanup;

    if ((domain = gvir_connection_find_domain_by_name(
             connection,
             gvir_sandbox_config_get_name(config))) != NULL) {
        if (!gvir_domain_delete(domain, 0, error))
            ret = FALSE;
    }

    if (!(builder = gvir_sandbox_builder_for_connection(connection,
                                                        error))) {
        ret = FALSE;
        goto cleanup;
    }

    if (unlink(configfile) < 0 &&
        errno != ENOENT)
        ret = FALSE;

    if (rmdir(emptydir) < 0 &&
        errno != ENOENT)
        ret = FALSE;

    if (rmdir(configdir) < 0 &&
        errno != ENOENT)
        ret = FALSE;

    if (rmdir(statedir) < 0 &&
        errno != ENOENT)
        ret = FALSE;

    if (!gvir_sandbox_builder_clean_post_stop(builder,
                                              config,
                                              statedir,
                                              error))
        ret = FALSE;

cleanup:
    g_free(servicedir);
    g_free(sandboxdir);
    g_free(statedir);
    g_free(configdir);
    g_free(configfile);
    g_free(emptydir);
    if (domain)
        g_object_unref(domain);
    if (builder)
        g_object_unref(builder);
    if (connection)
        g_object_unref(connection);
    if (config)
        g_object_unref(config);
    return ret;
}


static gboolean gvir_sandbox_context_service_start(GVirSandboxContext *ctxt, GError **error)
{
    GVirConnection *connection = NULL;
    GVirDomain *domain = NULL;
    GVirSandboxConfig *config = NULL;
    gboolean ret = FALSE;

    if (!GVIR_SANDBOX_CONTEXT_CLASS(gvir_sandbox_context_service_parent_class)->start(ctxt, error))
        return FALSE;

    connection = gvir_sandbox_context_get_connection(ctxt);
    config = gvir_sandbox_context_get_config(ctxt);

    if (!gvir_connection_fetch_domains(connection, NULL, error))
        goto cleanup;

    if (!(domain = gvir_connection_find_domain_by_name(
              connection,
              gvir_sandbox_config_get_name(config)))) {
        *error = g_error_new(GVIR_SANDBOX_CONTEXT_SERVICE_ERROR, 0,
                             "Sandbox %s does not exist",
                             gvir_sandbox_config_get_name(config));
        goto cleanup;
    }

    if (!gvir_domain_start(domain, 0, error))
        goto cleanup;

    g_object_set(ctxt, "domain", domain, NULL);

    ret = TRUE;
cleanup:
    g_object_unref(config);
    g_object_unref(connection);
    if (domain)
        g_object_unref(domain);
    return ret;
}


static gboolean gvir_sandbox_context_service_stop(GVirSandboxContext *ctxt, GError **error)
{
    GVirDomain *domain;
    gboolean ret = TRUE;

    if (!GVIR_SANDBOX_CONTEXT_CLASS(gvir_sandbox_context_service_parent_class)->stop(ctxt, error))
        return FALSE;

    if (!(domain = gvir_sandbox_context_get_domain(ctxt, error)))
        return FALSE;

    if (!gvir_domain_stop(domain, 0, error))
        ret = FALSE;

    g_object_set(ctxt, "domain", NULL, NULL);
    g_object_unref(domain);

    return ret;
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

    context_class->start = gvir_sandbox_context_service_start;
    context_class->stop = gvir_sandbox_context_service_stop;

    klass->define = gvir_sandbox_context_service_define_default;
    klass->undefine = gvir_sandbox_context_service_undefine_default;

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


gboolean gvir_sandbox_context_service_define(GVirSandboxContextService *ctxt, GError **error)
{
    return GVIR_SANDBOX_CONTEXT_SERVICE_GET_CLASS(ctxt)->define(ctxt, error);
}


gboolean gvir_sandbox_context_service_undefine(GVirSandboxContextService *ctxt, GError **error)
{
    return GVIR_SANDBOX_CONTEXT_SERVICE_GET_CLASS(ctxt)->undefine(ctxt, error);
}
