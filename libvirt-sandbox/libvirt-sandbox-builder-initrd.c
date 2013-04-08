/*
 * libvirt-sandbox-builder-initrd.c: libvirt sandbox configuration
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
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>

#include <glib/gi18n.h>

#include "libvirt-sandbox/libvirt-sandbox.h"

/**
 * SECTION: libvirt-sandbox-builder-initrd
 * @short_description: Kernel ramdisk construction
 * @include: libvirt-sandbox/libvirt-sandbox.h
 * @see_also: #GVirSandboxBuilderMachine, #GVirSandboxConfigInitrd
 *
 * Provides an object for constructing kernel ramdisks
 *
 * The GVirSandboxBuilderInitrd object provides the support
 * required to dynically creat minimal footprint kernel
 * ramdisks for booting virtual machine based sandboxes.
 */

#define GVIR_SANDBOX_BUILDER_INITRD_GET_PRIVATE(obj)                         \
        (G_TYPE_INSTANCE_GET_PRIVATE((obj), GVIR_SANDBOX_TYPE_BUILDER_INITRD, GVirSandboxBuilderInitrdPrivate))

struct _GVirSandboxBuilderInitrdPrivate
{
    gboolean unused;
};

G_DEFINE_TYPE(GVirSandboxBuilderInitrd, gvir_sandbox_builder_initrd, G_TYPE_OBJECT);


enum {
    PROP_0,
};

enum {
    LAST_SIGNAL
};

//static gint signals[LAST_SIGNAL];

#define GVIR_SANDBOX_BUILDER_INITRD_ERROR gvir_sandbox_builder_initrd_error_quark()

static GQuark
gvir_sandbox_builder_initrd_error_quark(void)
{
    return g_quark_from_static_string("gvir-sandbox-builder-initrd");
}


static void gvir_sandbox_builder_initrd_get_property(GObject *object,
                                                      guint prop_id,
                                                      GValue *value G_GNUC_UNUSED,
                                                      GParamSpec *pspec)
{
#if 0
    GVirSandboxBuilderInitrd *builder = GVIR_SANDBOX_BUILDER_INITRD(object);
    GVirSandboxBuilderInitrdPrivate *priv = builder->priv;
#endif

    switch (prop_id) {

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_builder_initrd_set_property(GObject *object,
                                                      guint prop_id,
                                                      const GValue *value G_GNUC_UNUSED,
                                                      GParamSpec *pspec)
{
#if 0
    GVirSandboxBuilderInitrd *builder = GVIR_SANDBOX_BUILDER_INITRD(object);
    GVirSandboxBuilderInitrdPrivate *priv = builder->priv;
#endif

    switch (prop_id) {

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_builder_initrd_finalize(GObject *object)
{
#if 0
    GVirSandboxBuilderInitrd *builder = GVIR_SANDBOX_BUILDER_INITRD(object);
    GVirSandboxBuilderInitrdPrivate *priv = builder->priv;
#endif

    G_OBJECT_CLASS(gvir_sandbox_builder_initrd_parent_class)->finalize(object);
}


static void gvir_sandbox_builder_initrd_class_init(GVirSandboxBuilderInitrdClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = gvir_sandbox_builder_initrd_finalize;
    object_class->get_property = gvir_sandbox_builder_initrd_get_property;
    object_class->set_property = gvir_sandbox_builder_initrd_set_property;

    g_type_class_add_private(klass, sizeof(GVirSandboxBuilderInitrdPrivate));
}


static void gvir_sandbox_builder_initrd_init(GVirSandboxBuilderInitrd *builder)
{
    builder->priv = GVIR_SANDBOX_BUILDER_INITRD_GET_PRIVATE(builder);
}


/**
 * gvir_sandbox_builder_initrd_new:
 *
 * Create a new initrd builder
 *
 * Returns: (transfer full): a new sandbox builder object
 */
GVirSandboxBuilderInitrd *gvir_sandbox_builder_initrd_new(void)
{
    return GVIR_SANDBOX_BUILDER_INITRD(g_object_new(GVIR_SANDBOX_TYPE_BUILDER_INITRD,
                                                     NULL));
}


static gchar *gvir_sandbox_builder_initrd_create_tempdir(GError **error)
{
    gchar *tmpl = NULL;
    gchar *dir = NULL;
    const gchar *tmpdir = getenv("TEMPDIR");

    if (!tmpdir)
        tmpdir = "/tmp";

    if (!(tmpl = g_strdup_printf("%s/libvirt-sandbox-XXXXXX", tmpdir)))
        goto cleanup;

    dir = mkdtemp(tmpl);

    if (!dir) {
        g_set_error(error, GVIR_SANDBOX_BUILDER_INITRD_ERROR, errno,
                    _("Unable to create temporary directory %s: %s"),
                    tmpl, strerror(errno));
        goto cleanup;
    }

cleanup:
    return dir;
}


static gboolean gvir_sandbox_builder_initrd_copy_file(const gchar *srcpath,
                                                      const gchar *tgtdir,
                                                      const gchar *tgtname,
                                                      GError **error)
{
    gchar *tgtpath = g_strdup_printf("%s/%s", tgtdir, tgtname);
    GFile *tgt = g_file_new_for_path(tgtpath);
    GFile *src = g_file_new_for_path(srcpath);
    gboolean ret = FALSE;

    g_free(tgtpath);

    if (!g_file_copy(src, tgt, 0, NULL, NULL, NULL, error))
        goto cleanup;

    ret = TRUE;

cleanup:
    g_object_unref(tgt);
    g_object_unref(src);
    return ret;
}

#define FIND_USING_GIO

#ifdef FIND_USING_GIO
static GList *gvir_sandbox_builder_initrd_find_files(GList *modnames,
                                                     GFile *dir,
                                                     GError **error)
{
    GFileEnumerator *children = g_file_enumerate_children(dir,
                                                          "standard::name,standard::type",
                                                          G_FILE_QUERY_INFO_NONE,
                                                          NULL,
                                                          error);
    GFileInfo *childinfo;
    GList *modfiles = NULL;
    if (!children)
        return NULL;

    while ((childinfo = g_file_enumerator_next_file(children, NULL, error)) != NULL) {
        const gchar *thisname = g_file_info_get_name(childinfo);
        GFile *child = g_file_get_child(dir, thisname);
        if (strstr(thisname, ".ko")) {
            GList *tmp = modnames;
            while (tmp) {
                if (g_str_equal(thisname, tmp->data)) {
                    modfiles = g_list_append(modfiles, child);
                    child = NULL;
                }
                tmp = tmp->next;
            }
        } else if (g_file_info_get_file_type(childinfo) == G_FILE_TYPE_DIRECTORY) {
            GList *newmodfiles =
                gvir_sandbox_builder_initrd_find_files(modnames, child, error);
            GList *tmp = newmodfiles;
            while (tmp) {
                modfiles = g_list_append(modfiles, tmp->data);
                tmp = tmp->next;
            }
            g_list_free(newmodfiles);
            if (*error)
                goto cleanup;
        }
        if (child)
            g_object_unref(child);
        g_object_unref(childinfo);
    }

cleanup:
    if (*error) {
        g_list_foreach(modfiles, (GFunc)g_object_unref, NULL);
        g_list_free(modfiles);
        modfiles = NULL;
    }
    g_object_unref(children);
    return modfiles;
}
#else
static GList *gvir_sandbox_builder_initrd_find_files(GList *modnames,
                                                     GFile *dir,
                                                     GError **error)
{
    DIR *dh = opendir(g_file_get_path(dir));
    if (!dh) {
        g_set_error(error, GVIR_SANDBOX_BUILDER_INITRD_ERROR, 0,
                    _("Unable to read entries in %s: %s"),
                    g_file_get_path(dir), strerror(errno));
        return NULL;
    }
    struct dirent *de;
    GList *modfiles = NULL;

    while ((de = readdir(dh)) != NULL) {
        gchar *childpath;
        GFile *child;
        if (de->d_name[0] == '.')
            continue;

        childpath = g_strdup_printf("%s/%s",
                                    g_file_get_path(dir),
                                    de->d_name);
        child = g_file_new_for_path(childpath);
        g_free(childpath);
        if (strstr(de->d_name, ".ko")) {
            GList *tmp = modnames;
            while (tmp) {
                if (g_str_equal(de->d_name, tmp->data)) {
                    modfiles = g_list_append(modfiles, child);
                    child = NULL;
                }
                tmp = tmp->next;
            }
        } else {
            struct stat sb;
            if (stat(g_file_get_path(child), &sb) < 0) {
                g_set_error(error, GVIR_SANDBOX_BUILDER_INITRD_ERROR, 0,
                            _("Unable to access %s: %s"),
                            g_file_get_path(child) , strerror(errno));
                g_object_unref(child);
                goto cleanup;
            }
            if (S_ISDIR(sb.st_mode)) {
                GList *newmodfiles =
                    gvir_sandbox_builder_initrd_find_files(modnames, child, error);
                GList *tmp = newmodfiles;
                while (tmp) {
                    modfiles = g_list_append(modfiles, tmp->data);
                    tmp = tmp->next;
                }
                g_list_free(newmodfiles);
                if (*error)
                    goto cleanup;
            }
        }
        if (child)
            g_object_unref(child);
    }

cleanup:
    if (*error) {
        g_list_foreach(modfiles, (GFunc)g_object_unref, NULL);
        g_list_free(modfiles);
        modfiles = NULL;
    }
    closedir(dh);
    return modfiles;
}
#endif


static GList *gvir_sandbox_builder_initrd_find_modules(GList *modnames,
                                                       GVirSandboxConfigInitrd *config,
                                                       GError **error)
{
    gchar *moddirpath = g_strdup_printf("%s",
                                        gvir_sandbox_config_initrd_get_kmoddir(config));
    GFile *moddir = g_file_new_for_path(moddirpath);

    GList *modfiles = gvir_sandbox_builder_initrd_find_files(modnames,
                                                             moddir,
                                                             error);

    GList *tmp1 = modnames;

    if (!modfiles)
        goto cleanup;

    while (tmp1) {
        gboolean found = FALSE;
        GList *tmp2 = modfiles;
        while (tmp2) {
            if (g_str_equal(g_file_get_basename(tmp2->data), tmp1->data))
                found = TRUE;
            tmp2 = tmp2->next;
        }
        if (!found) {
            g_set_error(error, GVIR_SANDBOX_BUILDER_INITRD_ERROR, 0,
                        _("Cannot find module file for %s"), (const gchar*)tmp1->data);
            g_list_foreach(modfiles, (GFunc)g_object_unref, NULL);
            g_list_free(modfiles);
            modfiles = NULL;
            goto cleanup;
        }

        tmp1 = tmp1->next;
    }

cleanup:
    g_free(moddirpath);
    g_object_unref(moddir);
    return modfiles;
}


static gboolean gvir_sandbox_builder_initrd_populate_tmpdir(const gchar *tmpdir,
                                                            GVirSandboxConfigInitrd *config,
                                                            GError **error)
{
    gboolean ret = FALSE;
    GList *modnames = NULL;
    GList *modfiles = NULL;
    GList *tmp;
    GFile *modlist = NULL;
    gchar *modlistpath = NULL;
    GOutputStream *modlistos = NULL;
    GError *e = NULL;

    if (!gvir_sandbox_builder_initrd_copy_file(
            gvir_sandbox_config_initrd_get_init(config),
            tmpdir, "init", error))
        return FALSE;

    modnames = gvir_sandbox_config_initrd_get_modules(config);
    modfiles = gvir_sandbox_builder_initrd_find_modules(modnames, config, &e);
    if (e) {
        g_propagate_error(error, e);
        goto cleanup;
    }

    tmp = modfiles;
    while (tmp) {
        const gchar *basename = g_file_get_basename(tmp->data);
        gchar *tgt = g_strdup_printf("%s/%s", tmpdir, basename);
        GFile *file = g_file_new_for_path(tgt);
        g_free(tgt);

        if (!g_file_copy(tmp->data, file, 0, NULL, NULL, NULL, error)) {
            g_object_unref(file);
            goto cleanup;
        }
        g_object_unref(file);

        tmp = tmp->next;
    }


    modlistpath = g_strdup_printf("%s/modules", tmpdir);
    modlist = g_file_new_for_path(modlistpath);
    if (!(modlistos = G_OUTPUT_STREAM(g_file_create(modlist, G_FILE_CREATE_NONE, NULL, error))))
        goto cleanup;

    tmp = modnames;
    while (tmp) {
        if (!g_output_stream_write_all(modlistos,
                                       tmp->data, strlen(tmp->data),
                                       NULL, NULL, error))
            goto cleanup;
        if (!g_output_stream_write_all(modlistos,
                                       "\n", 1,
                                       NULL, NULL, error))
            goto cleanup;
        tmp = tmp->next;
    }

    if (!g_output_stream_close(modlistos, NULL, error))
        goto cleanup;

    ret = TRUE;
cleanup:
    g_list_foreach(modfiles, (GFunc)g_object_unref, NULL);
    g_list_free(modfiles);
    g_list_free(modnames);
    g_free(modlistpath);
    if (modlist)
        g_object_unref(modlist);
    if (modlistos)
        g_object_unref(modlistos);
    return ret;
}


static gboolean gvir_sandbox_builder_initrd_create_initrd(const gchar *tmpdir,
                                                          const gchar *outputfile,
                                                          GError **error)
{
    gboolean ret = FALSE;
    gchar *src = g_shell_quote(tmpdir);
    gchar *tgt = g_shell_quote(outputfile);

    gchar *cmd = g_strdup_printf(
        "/bin/sh -c \"(cd %s && ( find | cpio --quiet -o -H newc | gzip -9 ) ) > %s\"",
        src, tgt);

    if (!g_spawn_command_line_sync(cmd, NULL, NULL, NULL, error))
        goto cleanup;

    ret = TRUE;
cleanup:
    g_free(src);
    g_free(tgt);
    g_free(cmd);
    return ret;
}


static gboolean gvir_sandbox_builder_initrd_remove_tmpdir(const gchar *tmpdir,
                                                          GError **error)
{
    GFile *dir = g_file_new_for_path(tmpdir);
    GFileEnumerator *children = g_file_enumerate_children(dir,
                                                          "standard::name",
                                                          G_FILE_QUERY_INFO_NONE,
                                                          NULL,
                                                          error);
    GFileInfo *childinfo;
    gboolean ret = FALSE;
    if (!children)
        goto cleanup;

    while ((childinfo = g_file_enumerator_next_file(children, NULL, error)) != NULL) {
        GFile *file = g_file_get_child(dir, g_file_info_get_name(childinfo));
        g_file_delete(file, NULL, error && !*error ? error : NULL);
        g_object_unref(childinfo);
    }

    if (error && *error)
        goto cleanup;

    ret = TRUE;
cleanup:
    g_file_delete(dir, NULL, error && !*error ? error : NULL);
    g_object_unref(dir);

    g_object_unref(children);
    return ret;
}

gboolean gvir_sandbox_builder_initrd_construct(GVirSandboxBuilderInitrd *builder G_GNUC_UNUSED,
                                               GVirSandboxConfigInitrd *config,
                                               gchar *outputfile,
                                               GError **error)
{
    gchar *tmpdir = NULL;
    gboolean ret = FALSE;
    GFile *tgt = g_file_new_for_path(outputfile);
    mode_t mask;

    mask = umask(0077);

    if (!(tmpdir = gvir_sandbox_builder_initrd_create_tempdir(error)))
        goto cleanup;

    if (!gvir_sandbox_builder_initrd_populate_tmpdir(tmpdir, config, error))
        goto cleanup;

    if (!gvir_sandbox_builder_initrd_create_initrd(tmpdir, outputfile, error))
        goto cleanup;

    ret = TRUE;
cleanup:
    if (tmpdir &&
        !gvir_sandbox_builder_initrd_remove_tmpdir(tmpdir, ret ? error : NULL))
        ret = FALSE;

    if (!ret && tgt)
        g_file_delete(tgt, NULL, NULL);

    umask(mask);

    return ret;
}
