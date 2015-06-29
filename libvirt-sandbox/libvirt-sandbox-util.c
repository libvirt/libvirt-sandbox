/*
 * libvirt-sandbox-util.c: libvirt sandbox util functions
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

#include <config.h>
#include <string.h>
#include <errno.h>
#include <glib/gi18n.h>

#include "libvirt-sandbox/libvirt-sandbox-config-all.h"

#define GVIR_SANDBOX_UTIL_ERROR gvir_sandbox_util_error_quark()

static GQuark
gvir_sandbox_util_error_quark(void)
{
    return g_quark_from_static_string("gvir-sandbox-util");
}

gint gvir_sandbox_util_guess_image_format(const gchar *path,
                                          GError **error)
{
    gchar *tmp;

    if ((tmp = strchr(path, '.')) == NULL) {
        g_set_error(error, GVIR_SANDBOX_UTIL_ERROR, 0,
                    _("Cannot identify file extension in '%s'"), path);
        return -1;
    }
    tmp = tmp + 1;

    if (g_str_equal(tmp, "img"))
       return GVIR_CONFIG_DOMAIN_DISK_FORMAT_RAW;

    return gvir_sandbox_util_disk_format_from_str(tmp, error);
}

gint gvir_sandbox_util_disk_format_from_str(const gchar *value,
                                            GError **error)
{
    GEnumClass *enum_class = g_type_class_ref(GVIR_CONFIG_TYPE_DOMAIN_DISK_FORMAT);
    GEnumValue *enum_value;
    gint ret = -1;

    if (!(enum_value = g_enum_get_value_by_nick(enum_class, value))) {
        g_set_error(error, GVIR_SANDBOX_UTIL_ERROR, 0,
                    _("Unknown disk format '%s'"), value);
        goto cleanup;
    }
    ret = enum_value->value;

 cleanup:
    g_type_class_unref(enum_class);
    return ret;
}
