/*
 * libvirt-sandbox-config-mount-host-bind.c: libvirt sandbox configuration
 *
 * Copyright (C) 2011-2012 Red Hat, Inc.
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
 * SECTION: libvirt-sandbox-config-mount-host-bind
 * @short_description: Bindsystem attachment configuration details
 * @include: libvirt-sandbox/libvirt-sandbox.h
 * @see_aloso: #GVirSandboxConfig
 *
 * Provides an object to store information about a bindsystem attachment in the sandbox
 *
 * The GVirSandboxConfigMount object stores information required to attach
 * a host bindsystem to the application sandbox. The sandbox starts off with
 * a complete view of the host bindsystem. This object allows a specific
 * area of the host bindsystem to be hidden and replaced with alternate
 * content.
 */

#define GVIR_SANDBOX_CONFIG_MOUNT_HOST_BIND_GET_PRIVATE(obj)                         \
        (G_TYPE_INSTANCE_GET_PRIVATE((obj), GVIR_SANDBOX_TYPE_CONFIG_MOUNT_HOST_BIND, GVirSandboxConfigMountHostBindPrivate))

struct _GVirSandboxConfigMountHostBindPrivate
{
    gboolean unused;
};

G_DEFINE_TYPE(GVirSandboxConfigMountHostBind, gvir_sandbox_config_mount_host_bind, GVIR_SANDBOX_TYPE_CONFIG_MOUNT_FILE);


static void gvir_sandbox_config_mount_host_bind_class_init(GVirSandboxConfigMountHostBindClass *klass)
{
    g_type_class_add_private(klass, sizeof(GVirSandboxConfigMountHostBindPrivate));
}


static void gvir_sandbox_config_mount_host_bind_init(GVirSandboxConfigMountHostBind *config)
{
    config->priv = GVIR_SANDBOX_CONFIG_MOUNT_HOST_BIND_GET_PRIVATE(config);
}


/**
 * gvir_sandbox_config_mount_host_bind_new:
 * @targetdir: (transfer none): the target directory
 *
 * Create a new custom mount mapped to the directory @targetdir
 *
 * Returns: (transfer full): a new sandbox mount object
 */
GVirSandboxConfigMountHostBind *gvir_sandbox_config_mount_host_bind_new(const gchar *source,
                                                                        const gchar *targetdir)
{
    return GVIR_SANDBOX_CONFIG_MOUNT_HOST_BIND(g_object_new(GVIR_SANDBOX_TYPE_CONFIG_MOUNT_HOST_BIND,
                                                            "source", source,
                                                            "target", targetdir,
                                                            NULL));
}
