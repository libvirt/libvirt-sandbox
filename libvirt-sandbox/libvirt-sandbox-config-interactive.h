/*
 * libvirt-sandbox-config-interactive.h: libvirt sandbox configuration
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

#ifndef __LIBVIRT_SANDBOX_CONFIG_INTERACTIVE_H__
#define __LIBVIRT_SANDBOX_CONFIG_INTERACTIVE_H__

G_BEGIN_DECLS

#define GVIR_SANDBOX_TYPE_CONFIG_INTERACTIVE            (gvir_sandbox_config_interactive_get_type ())
#define GVIR_SANDBOX_CONFIG_INTERACTIVE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GVIR_SANDBOX_TYPE_CONFIG_INTERACTIVE, GVirSandboxConfigInteractive))
#define GVIR_SANDBOX_CONFIG_INTERACTIVE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GVIR_SANDBOX_TYPE_CONFIG_INTERACTIVE, GVirSandboxConfigInteractiveClass))
#define GVIR_SANDBOX_IS_CONFIG_INTERACTIVE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GVIR_SANDBOX_TYPE_CONFIG_INTERACTIVE))
#define GVIR_SANDBOX_IS_CONFIG_INTERACTIVE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GVIR_SANDBOX_TYPE_CONFIG_INTERACTIVE))
#define GVIR_SANDBOX_CONFIG_INTERACTIVE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GVIR_SANDBOX_TYPE_CONFIG_INTERACTIVE, GVirSandboxConfigInteractiveClass))

typedef struct _GVirSandboxConfigInteractive GVirSandboxConfigInteractive;
typedef struct _GVirSandboxConfigInteractivePrivate GVirSandboxConfigInteractivePrivate;
typedef struct _GVirSandboxConfigInteractiveClass GVirSandboxConfigInteractiveClass;

struct _GVirSandboxConfigInteractive
{
    GVirSandboxConfig parent;

    GVirSandboxConfigInteractivePrivate *priv;

    /* Do not add fields to this struct */
};

struct _GVirSandboxConfigInteractiveClass
{
    GVirSandboxConfigClass parent_class;

    gpointer padding[LIBVIRT_SANDBOX_CLASS_PADDING];
};

GType gvir_sandbox_config_interactive_get_type(void);

GVirSandboxConfigInteractive *gvir_sandbox_config_interactive_new(const gchar *name);

void gvir_sandbox_config_interactive_set_tty(GVirSandboxConfigInteractive *config, gboolean tty);
gboolean gvir_sandbox_config_interactive_get_tty(GVirSandboxConfigInteractive *config);

void gvir_sandbox_config_interactive_set_command(GVirSandboxConfigInteractive *config, gchar **argv);


G_END_DECLS

#endif /* __LIBVIRT_SANDBOX_CONFIG_INTERACTIVE_H__ */
