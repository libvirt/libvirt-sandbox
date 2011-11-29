/*
 * libvirt-sandbox-config.h: libvirt sandbox configuration
 *
 * Copyright (C) 2011 Red Hat
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
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

#define GVIR_SANDBOX_TYPE_CONFIG_HANDLE      (gvir_sandbox_config_handle_get_type ())

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

};

GType gvir_sandbox_config_get_type(void);

GVirSandboxConfig *gvir_sandbox_config_new(const gchar *name);

const gchar *gvir_sandbox_config_get_name(GVirSandboxConfig *config);

void gvir_sandbox_config_set_root(GVirSandboxConfig *config, const gchar *hostdir);
const gchar *gvir_sandbox_config_get_root(GVirSandboxConfig *config);

void gvir_sandbox_config_set_arch(GVirSandboxConfig *config, const gchar *arch);
const gchar *gvir_sandbox_config_get_arch(GVirSandboxConfig *config);

void gvir_sandbox_config_set_tty(GVirSandboxConfig *config, gboolean tty);
gboolean gvir_sandbox_config_get_tty(GVirSandboxConfig *config);

void gvir_sandbox_config_set_userid(GVirSandboxConfig *config, guint uid);
guint gvir_sandbox_config_get_userid(GVirSandboxConfig *config);

void gvir_sandbox_config_set_groupid(GVirSandboxConfig *config, guint gid);
guint gvir_sandbox_config_get_groupid(GVirSandboxConfig *config);

void gvir_sandbox_config_set_username(GVirSandboxConfig *config, const gchar *username);
const gchar *gvir_sandbox_config_get_username(GVirSandboxConfig *config);

void gvir_sandbox_config_set_homedir(GVirSandboxConfig *config, const gchar *homedir);
const gchar *gvir_sandbox_config_get_homedir(GVirSandboxConfig *config);

void gvir_sandbox_config_add_mount(GVirSandboxConfig *config,
                                   GVirSandboxConfigMount *mnt);
GList *gvir_sandbox_config_get_mounts(GVirSandboxConfig *config);
GVirSandboxConfigMount *gvir_sandbox_config_find_mount(GVirSandboxConfig *config,
                                                       const gchar *target);
void gvir_sandbox_config_add_mount_strv(GVirSandboxConfig *config,
                                        gchar **mounts);

gboolean gvir_sandbox_config_add_include_strv(GVirSandboxConfig *config,
                                              gchar **includes,
                                              GError **error);
gboolean gvir_sandbox_config_add_include_file(GVirSandboxConfig *config,
                                              gchar *includefile,
                                              GError **error);

void gvir_sandbox_config_set_command(GVirSandboxConfig *config, gchar **argv);
gchar **gvir_sandbox_config_get_command(GVirSandboxConfig *config);

void gvir_sandbox_config_set_security_label(GVirSandboxConfig *config, const gchar *label);
const gchar *gvir_sandbox_config_get_security_label(GVirSandboxConfig *config);
void gvir_sandbox_config_set_security_dynamic(GVirSandboxConfig *config, gboolean dynamic);
gboolean gvir_sandbox_config_get_security_dynamic(GVirSandboxConfig *config);

G_END_DECLS

#endif /* __LIBVIRT_SANDBOX_CONFIG_H__ */
