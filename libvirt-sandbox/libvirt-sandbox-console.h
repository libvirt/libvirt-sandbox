/*
 * libvirt-sandbox-console.h: libvirt sandbox console
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

#ifndef __LIBVIRT_SANDBOX_CONSOLE_H__
#define __LIBVIRT_SANDBOX_CONSOLE_H__

#include <gio/gunixinputstream.h>
#include <gio/gunixoutputstream.h>

G_BEGIN_DECLS

#define GVIR_SANDBOX_TYPE_CONSOLE            (gvir_sandbox_console_get_type ())
#define GVIR_SANDBOX_CONSOLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GVIR_SANDBOX_TYPE_CONSOLE, GVirSandboxConsole))
#define GVIR_SANDBOX_IS_CONSOLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GVIR_SANDBOX_TYPE_CONSOLE))
#define GVIR_SANDBOX_CONSOLE_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), GVIR_SANDBOX_TYPE_CONSOLE, GVirSandboxConsoleInterface))

#define GVIR_SANDBOX_TYPE_CONSOLE_HANDLE      (gvir_sandbox_console_handle_get_type ())

typedef struct _GVirSandboxConsole GVirSandboxConsole; /* dummy object */
typedef struct _GVirSandboxConsoleInterface GVirSandboxConsoleInterface;

struct _GVirSandboxConsoleInterface
{
    GTypeInterface parent;

    gboolean (*attach)(GVirSandboxConsole *console,
                       GUnixInputStream *localStdin,
                       GUnixOutputStream *localStdout,
                       GUnixOutputStream *localStderr,
                       GError **error);
    gboolean (*detach)(GVirSandboxConsole *console,
                       GError **error);

    /* Do not add fields to this struct */
};

GType gvir_sandbox_console_get_type(void);


gboolean gvir_sandbox_console_attach_stdio(GVirSandboxConsole *console,
                                           GError **error);

gboolean gvir_sandbox_console_attach(GVirSandboxConsole *console,
                                     GUnixInputStream *localStdin,
                                     GUnixOutputStream *localStdout,
                                     GUnixOutputStream *localStderr,
                                     GError **error);

gboolean gvir_sandbox_console_detach(GVirSandboxConsole *console,
                                     GError **error);

G_END_DECLS

#endif /* __LIBVIRT_SANDBOX_CONSOLE_H__ */
