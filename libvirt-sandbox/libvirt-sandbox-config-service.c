/*
 * libvirt-sandbox-config-service.c: libvirt sandbox configuration
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
 * SECTION: libvirt-sandbox-config-service
 * @short_description: Service sandbox configuration details
 * @include: libvirt-sandbox/libvirt-sandbox.h
 * @see_also: #GVirSandboxConfigService
 *
 * Provides an object to store configuration details for a service config
 *
 * The GVirSandboxConfigService object extends #GVirSandboxConfig to store
 * the extra information required to setup a service sandbox
 */

#define GVIR_SANDBOX_CONFIG_SERVICE_GET_PRIVATE(obj)                         \
        (G_TYPE_INSTANCE_GET_PRIVATE((obj), GVIR_SANDBOX_TYPE_CONFIG_SERVICE, GVirSandboxConfigServicePrivate))

struct _GVirSandboxConfigServicePrivate
{
    gboolean unused;
};

G_DEFINE_ABSTRACT_TYPE(GVirSandboxConfigService, gvir_sandbox_config_service, GVIR_SANDBOX_TYPE_CONFIG);


enum {
    LAST_SIGNAL
};

//static gint signals[LAST_SIGNAL];

#if 0
#define GVIR_SANDBOX_CONFIG_SERVICE_ERROR gvir_sandbox_config_service_error_quark()

static GQuark
gvir_sandbox_config_service_error_quark(void)
{
    return g_quark_from_static_string("gvir-sandbox-config-service");
}
#endif


static void gvir_sandbox_config_service_finalize(GObject *object)
{

    G_OBJECT_CLASS(gvir_sandbox_config_service_parent_class)->finalize(object);
}


static void gvir_sandbox_config_service_class_init(GVirSandboxConfigServiceClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = gvir_sandbox_config_service_finalize;

    g_type_class_add_private(klass, sizeof(GVirSandboxConfigServicePrivate));
}


static void gvir_sandbox_config_service_init(GVirSandboxConfigService *config)
{
    config->priv = GVIR_SANDBOX_CONFIG_SERVICE_GET_PRIVATE(config);
}
