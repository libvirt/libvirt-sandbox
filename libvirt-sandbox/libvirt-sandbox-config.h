/*
 * libvirt-sandbox-config.h: libvirt sandbox configuration
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

#ifndef __LIBVIRT_SANDBOX_CONFIG_H__
#define __LIBVIRT_SANDBOX_CONFIG_H__

G_BEGIN_DECLS

#define GVIR_SANDBOX_TYPE_CONFIG            (gvir_sandbox_config_get_type ())
#define GVIR_SANDBOX_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GVIR_SANDBOX_TYPE_CONFIG, GVirSandboxConfig))
#define GVIR_SANDBOX_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GVIR_SANDBOX_TYPE_CONFIG, GVirSandboxConfigClass))
#define GVIR_SANDBOX_IS_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GVIR_SANDBOX_TYPE_CONFIG))
#define GVIR_SANDBOX_IS_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GVIR_SANDBOX_TYPE_CONFIG))
#define GVIR_SANDBOX_CONFIG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GVIR_SANDBOX_TYPE_CONFIG, GVirSandboxConfigClass))

typedef struct _GVirSandboxConfig GVirSandboxConfig;
typedef struct _GVirSandboxConfigPrivate GVirSandboxConfigPrivate;
typedef struct _GVirSandboxConfigClass GVirSandboxConfigClass;

struct _GVirSandboxConfig
{
    GObject parent;

    GVirSandboxConfigPrivate *priv;

    /* Do not add fields to this struct */
};

struct _GVirSandboxConfigClass
{
    GObjectClass parent_class;

    gboolean (*load_config)(GVirSandboxConfig *config,
                            GKeyFile *file,
                            GError **error);
    void (*save_config)(GVirSandboxConfig *config,
                        GKeyFile *file);

    gchar **(*get_command)(GVirSandboxConfig *config);

    gpointer padding[LIBVIRT_SANDBOX_CLASS_PADDING];
};

GType gvir_sandbox_config_get_type(void);

GVirSandboxConfig *gvir_sandbox_config_load_from_path(const gchar *path,
                                                      GError **error);
GVirSandboxConfig *gvir_sandbox_config_load_from_data(const gchar *data,
                                                      GError **error);
gboolean gvir_sandbox_config_save_to_path(GVirSandboxConfig *config,
                                          const gchar *path,
                                          GError **error);
gchar *gvir_sandbox_config_save_to_data(GVirSandboxConfig *config,
                                        GError **error);

const gchar *gvir_sandbox_config_get_name(GVirSandboxConfig *config);

void gvir_sandbox_config_set_uuid(GVirSandboxConfig *config, const gchar *uuid);
const gchar *gvir_sandbox_config_get_uuid(GVirSandboxConfig *config);

void gvir_sandbox_config_set_root(GVirSandboxConfig *config, const gchar *hostdir);
const gchar *gvir_sandbox_config_get_root(GVirSandboxConfig *config);

void gvir_sandbox_config_set_arch(GVirSandboxConfig *config, const gchar *arch);
const gchar *gvir_sandbox_config_get_arch(GVirSandboxConfig *config);

void gvir_sandbox_config_set_kernrelease(GVirSandboxConfig *config, const gchar *kernrelease);
const gchar *gvir_sandbox_config_get_kernrelease(GVirSandboxConfig *config);

void gvir_sandbox_config_set_kernpath(GVirSandboxConfig *config, const gchar *kernpath);
const gchar *gvir_sandbox_config_get_kernpath(GVirSandboxConfig *config);

void gvir_sandbox_config_set_kmodpath(GVirSandboxConfig *config, const gchar *kmodpath);
const gchar *gvir_sandbox_config_get_kmodpath(GVirSandboxConfig *config);

void gvir_sandbox_config_set_shell(GVirSandboxConfig *config, gboolean shell);
gboolean gvir_sandbox_config_get_shell(GVirSandboxConfig *config);

void gvir_sandbox_config_set_userid(GVirSandboxConfig *config, guint uid);
guint gvir_sandbox_config_get_userid(GVirSandboxConfig *config);

void gvir_sandbox_config_set_groupid(GVirSandboxConfig *config, guint gid);
guint gvir_sandbox_config_get_groupid(GVirSandboxConfig *config);

void gvir_sandbox_config_set_username(GVirSandboxConfig *config, const gchar *username);
const gchar *gvir_sandbox_config_get_username(GVirSandboxConfig *config);

void gvir_sandbox_config_set_homedir(GVirSandboxConfig *config, const gchar *homedir);
const gchar *gvir_sandbox_config_get_homedir(GVirSandboxConfig *config);

void gvir_sandbox_config_add_network(GVirSandboxConfig *config,
                                     GVirSandboxConfigNetwork *network);
GList *gvir_sandbox_config_get_networks(GVirSandboxConfig *config);
gboolean gvir_sandbox_config_add_network_opts(GVirSandboxConfig *config,
                                              const gchar *network,
                                              GError **error);
gboolean gvir_sandbox_config_add_network_strv(GVirSandboxConfig *config,
                                              gchar **networks,
                                              GError **error);
gboolean gvir_sandbox_config_has_networks(GVirSandboxConfig *config);

void gvir_sandbox_config_add_mount(GVirSandboxConfig *config,
                                   GVirSandboxConfigMount *mnt);
GList *gvir_sandbox_config_get_mounts(GVirSandboxConfig *config);
GList *gvir_sandbox_config_get_mounts_with_type(GVirSandboxConfig *config,
                                                GType type);
GVirSandboxConfigMount *gvir_sandbox_config_find_mount(GVirSandboxConfig *config,
                                                       const gchar *target);

gboolean gvir_sandbox_config_add_mount_opts(GVirSandboxConfig *config,
                                            const char *mount,
                                            GError **error);
gboolean gvir_sandbox_config_add_mount_strv(GVirSandboxConfig *config,
                                            gchar **mounts,
                                            GError **error);
gboolean gvir_sandbox_config_has_mounts(GVirSandboxConfig *config);
gboolean gvir_sandbox_config_has_mounts_with_type(GVirSandboxConfig *config,
                                                  GType type);

gboolean gvir_sandbox_config_add_host_include_strv(GVirSandboxConfig *config,
                                                   gchar **includes,
                                                   GError **error);
gboolean gvir_sandbox_config_add_host_include_file(GVirSandboxConfig *config,
                                                   gchar *includefile,
                                                   GError **error);

void gvir_sandbox_config_set_security_label(GVirSandboxConfig *config, const gchar *label);
const gchar *gvir_sandbox_config_get_security_label(GVirSandboxConfig *config);
void gvir_sandbox_config_set_security_dynamic(GVirSandboxConfig *config, gboolean dynamic);
gboolean gvir_sandbox_config_get_security_dynamic(GVirSandboxConfig *config);
gboolean gvir_sandbox_config_set_security_opts(GVirSandboxConfig *config,
                                               const gchar *optstr,
                                               GError**error);

gchar **gvir_sandbox_config_get_command(GVirSandboxConfig *config);

G_END_DECLS

#endif /* __LIBVIRT_SANDBOX_CONFIG_H__ */
