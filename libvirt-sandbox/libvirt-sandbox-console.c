/*
 * libvirt-sandbox-console.c: libvirt sandbox console
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

#include "libvirt-sandbox/libvirt-sandbox.h"

/**
 * SECTION: libvirt-sandbox-console
 * @short_description: A text mode console
 * @include: libvirt-sandbox/libvirt-sandbox.h
 *
 * Provides an object to interface to the text mode console of the sandbox
 *
 * The GVirSandboxConsole object provides support for interfacing to the
 * text mode console of the sandbox. It forwards I/O between the #GVirStream
 * associated with the virtual machine's console and a local console
 * represented by #GUnixInputStream and #GUnixOutputStream objects.
 *
 */

gboolean gvir_sandbox_console_attach(GVirSandboxConsole *console,
                                     GUnixInputStream *localStdin,
                                     GUnixOutputStream *localStdout,
                                     GUnixOutputStream *localStderr,
                                     GError **error)
{
    return GVIR_SANDBOX_CONSOLE_GET_INTERFACE(console)->attach(console, localStdin, localStdout, localStderr, error);
}

gboolean gvir_sandbox_console_attach_stdio(GVirSandboxConsole *console_,
                                           GError **error)
{
    GInputStream *localStdin = g_unix_input_stream_new(STDIN_FILENO, FALSE);
    GOutputStream *localStdout = g_unix_output_stream_new(STDOUT_FILENO, FALSE);
    GOutputStream *localStderr = g_unix_output_stream_new(STDERR_FILENO, FALSE);
    gboolean ret;

    ret = gvir_sandbox_console_attach(console_,
                                      G_UNIX_INPUT_STREAM(localStdin),
                                      G_UNIX_OUTPUT_STREAM(localStdout),
                                      G_UNIX_OUTPUT_STREAM(localStderr),
                                      error);

    g_object_unref(localStdin);
    g_object_unref(localStdout);
    g_object_unref(localStderr);

    return ret;
}


gboolean gvir_sandbox_console_attach_stderr(GVirSandboxConsole *console_,
                                           GError **error)
{
    GOutputStream *localStderr = g_unix_output_stream_new(STDERR_FILENO, FALSE);
    gboolean ret;

    ret = gvir_sandbox_console_attach(console_,
                                      NULL, NULL,
                                      G_UNIX_OUTPUT_STREAM(localStderr),
                                      error);

    g_object_unref(localStderr);

    return ret;
}


gboolean gvir_sandbox_console_detach(GVirSandboxConsole *console,
                                     GError **error)
{
    return GVIR_SANDBOX_CONSOLE_GET_INTERFACE(console)->detach(console, error);
}

GType
gvir_sandbox_console_get_type (void)
{
    static GType console_type = 0;

    if (!console_type) {
        console_type =
            g_type_register_static_simple(G_TYPE_INTERFACE, "GVirSandboxConsole",
                                          sizeof (GVirSandboxConsoleInterface),
                                          NULL, 0, NULL, 0);

        g_type_interface_add_prerequisite(console_type, G_TYPE_OBJECT);
    }

    return console_type;
}
