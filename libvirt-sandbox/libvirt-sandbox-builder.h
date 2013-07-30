/*
 * libvirt-sandbox-builder.h: libvirt sandbox builder
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

#ifndef __LIBVIRT_SANDBOX_BUILDER_H__
#define __LIBVIRT_SANDBOX_BUILDER_H__

G_BEGIN_DECLS

#define GVIR_SANDBOX_TYPE_BUILDER            (gvir_sandbox_builder_get_type ())
#define GVIR_SANDBOX_BUILDER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GVIR_SANDBOX_TYPE_BUILDER, GVirSandboxBuilder))
#define GVIR_SANDBOX_BUILDER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GVIR_SANDBOX_TYPE_BUILDER, GVirSandboxBuilderClass))
#define GVIR_SANDBOX_IS_BUILDER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GVIR_SANDBOX_TYPE_BUILDER))
#define GVIR_SANDBOX_IS_BUILDER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GVIR_SANDBOX_TYPE_BUILDER))
#define GVIR_SANDBOX_BUILDER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GVIR_SANDBOX_TYPE_BUILDER, GVirSandboxBuilderClass))

#define GVIR_SANDBOX_TYPE_BUILDER_HANDLE      (gvir_sandbox_builder_handle_get_type ())

typedef struct _GVirSandboxBuilder GVirSandboxBuilder;
typedef struct _GVirSandboxBuilderPrivate GVirSandboxBuilderPrivate;
typedef struct _GVirSandboxBuilderClass GVirSandboxBuilderClass;

struct _GVirSandboxBuilder
{
    GObject parent;

    GVirSandboxBuilderPrivate *priv;

    /* Do not add fields to this struct */
};

struct _GVirSandboxBuilderClass
{
    GObjectClass parent_class;

    gboolean (*construct_basic)(GVirSandboxBuilder *builder,
                                GVirSandboxConfig *config,
                                const gchar *statedir,
                                GVirConfigDomain *domain,
                                GError **error);
    gboolean (*construct_os)(GVirSandboxBuilder *builder,
                             GVirSandboxConfig *config,
                             const gchar *statedir,
                             GVirConfigDomain *domain,
                             GError **error);
    gboolean (*construct_features)(GVirSandboxBuilder *builder,
                                   GVirSandboxConfig *config,
                                   const gchar *statedir,
                                   GVirConfigDomain *domain,
                                   GError **error);
    gboolean (*construct_devices)(GVirSandboxBuilder *builder,
                                  GVirSandboxConfig *config,
                                  const gchar *statedir,
                                  GVirConfigDomain *domain,
                                  GError **error);
    gboolean (*construct_security)(GVirSandboxBuilder *builder,
                                   GVirSandboxConfig *config,
                                   const gchar *statedir,
                                   GVirConfigDomain *domain,
                                   GError **error);
    gboolean (*construct_domain)(GVirSandboxBuilder *builder,
                                 GVirSandboxConfig *config,
                                 const gchar *statedir,
                                 GVirConfigDomain *domain,
                                 GError **error);

    gboolean (*clean_post_start)(GVirSandboxBuilder *builder,
                                 GVirSandboxConfig *config,
                                 const gchar *statedir,
                                 GError **error);
    gboolean (*clean_post_stop)(GVirSandboxBuilder *builder,
                                GVirSandboxConfig *config,
                                const gchar *statedir,
                                GError **error);

    gpointer padding[LIBVIRT_SANDBOX_CLASS_PADDING];
};

GType gvir_sandbox_builder_get_type(void);

GVirSandboxBuilder *gvir_sandbox_builder_for_connection(GVirConnection *connection,
                                                        GError **error);

GVirConnection *gvir_sandbox_builder_get_connection(GVirSandboxBuilder *builder);

GVirConfigDomain *gvir_sandbox_builder_construct(GVirSandboxBuilder *builder,
                                                 GVirSandboxConfig *config,
                                                 const gchar *statedir,
                                                 GError **error);

gboolean gvir_sandbox_builder_clean_post_start(GVirSandboxBuilder *builder,
                                               GVirSandboxConfig *config,
                                               const gchar *statedir,
                                               GError **error);
gboolean gvir_sandbox_builder_clean_post_stop(GVirSandboxBuilder *builder,
                                              GVirSandboxConfig *config,
                                              const gchar *statedir,
                                              GError **error);

G_END_DECLS

#endif /* __LIBVIRT_SANDBOX_BUILDER_H__ */
