/*
 * libvirt-sandbox-main.c: libvirt sandbox integration
 *
 * Copyright (C) 2008 Daniel P. Berrange
 * Copyright (C) 2010-2011 Red Hat, Inc.
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

#include <stdlib.h>
#include <stdio.h>

#include <glib/gi18n.h>

#include <libvirt-sandbox/libvirt-sandbox.h>
#include <libvirt-gconfig/libvirt-gconfig.h>

/**
 * gvir_sandbox_init:
 * @argc: (inout): pointer to application's argc
 * @argv: (inout) (array length=argc) (allow-none): pointer to application's argv
 */
void gvir_sandbox_init(int *argc,
                       char ***argv)
{
    GError *err = NULL;
    if (!gvir_sandbox_init_check(argc, argv, &err)) {
        g_error("Could not initialize libvirt-sandbox: %s\n",
                err->message);
    }
}

static void gvir_log_handler(const gchar *log_domain G_GNUC_UNUSED,
                             GLogLevelFlags log_level G_GNUC_UNUSED,
                             const gchar *message,
                             gpointer user_data)
{
    if (user_data)
        fprintf(stderr, "%s\n\r", message);
}


/**
 * gvir_sandbox_init_check:
 * @argc: (inout): pointer to application's argc
 * @argv: (inout) (array length=argc) (allow-none): pointer to application's argv
 * @err: pointer to a #GError to which a message will be posted on error
 */
gboolean gvir_sandbox_init_check(int *argc,
                                 char ***argv,
                                 GError **err)
{
    if (!gvir_init_object_check(argc, argv, err))
        return FALSE;

    if (!bindtextdomain(PACKAGE, LOCALEDIR))
        return FALSE;

    /* GLib >= 2.31.0 debug is off by default, so we need to
     * enable it. Older versions are on by default, so we need
     * to disable it.
     */
#if GLIB_CHECK_VERSION(2, 31, 0)
    if (getenv("LIBVIRT_SANDBOX_DEBUG"))
        g_log_set_handler(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
                          gvir_log_handler, (void*)0x1);
#else
    if (!getenv("LIBVIRT_SANDBOX_DEBUG"))
        g_log_set_handler(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
                          gvir_log_handler, NULL);
#endif

    return TRUE;
}
