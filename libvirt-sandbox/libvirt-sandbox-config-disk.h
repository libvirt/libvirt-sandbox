/*
 * libvirt-sandbox-config-diskaccess.h: libvirt sandbox configuration
 *
 * Copyright (C) 2015 Universitat Politècnica de Catalunya.
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
 * Author: Eren Yagdiran <erenyagdiran@gmail.com>
 */

#if !defined(__LIBVIRT_SANDBOX_H__) && !defined(LIBVIRT_SANDBOX_BUILD)
#error "Only <libvirt-sandbox/libvirt-sandbox.h> can be included directly."
#endif

#ifndef __LIBVIRT_SANDBOX_CONFIG_DISK_H__
#define __LIBVIRT_SANDBOX_CONFIG_DISK_H__

G_BEGIN_DECLS

#define GVIR_SANDBOX_TYPE_CONFIG_DISK            (gvir_sandbox_config_disk_get_type ())
#define GVIR_SANDBOX_CONFIG_DISK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GVIR_SANDBOX_TYPE_CONFIG_DISK, GVirSandboxConfigDisk))
#define GVIR_SANDBOX_CONFIG_DISK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GVIR_SANDBOX_TYPE_CONFIG_DISK, GVirSandboxConfigDiskClass))
#define GVIR_SANDBOX_IS_CONFIG_DISK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GVIR_SANDBOX_TYPE_CONFIG_DISK))
#define GVIR_SANDBOX_IS_CONFIG_DISK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GVIR_SANDBOX_TYPE_CONFIG_DISK))
#define GVIR_SANDBOX_CONFIG_DISK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GVIR_SANDBOX_TYPE_CONFIG_DISK, GVirSandboxConfigDiskClass))

#define GVIR_SANDBOX_TYPE_CONFIG_DISK_HANDLE      (gvir_sandbox_config_disk_handle_get_type ())

typedef struct _GVirSandboxConfigDisk GVirSandboxConfigDisk;
typedef struct _GVirSandboxConfigDiskPrivate GVirSandboxConfigDiskPrivate;
typedef struct _GVirSandboxConfigDiskClass GVirSandboxConfigDiskClass;

struct _GVirSandboxConfigDisk
{
    GObject parent;

    GVirSandboxConfigDiskPrivate *priv;

    /* Do not add fields to this struct */
};

struct _GVirSandboxConfigDiskClass
{
    GObjectClass parent_class;

    gpointer padding[LIBVIRT_SANDBOX_CLASS_PADDING];
};

GType gvir_sandbox_config_disk_get_type(void);

const gchar *gvir_sandbox_config_disk_get_tag(GVirSandboxConfigDisk *config);

const gchar *gvir_sandbox_config_disk_get_source(GVirSandboxConfigDisk *config);

GVirConfigDomainDiskType gvir_sandbox_config_disk_get_disk_type(GVirSandboxConfigDisk *config);

GVirConfigDomainDiskFormat gvir_sandbox_config_disk_get_format(GVirSandboxConfigDisk *config);

G_END_DECLS

#endif /* __LIBVIRT_SANDBOX_CONFIG_DISK_H__ */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
