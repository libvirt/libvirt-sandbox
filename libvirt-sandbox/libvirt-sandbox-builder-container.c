/*
 * libvirt-sandbox-builder-container.c: libvirt sandbox configuration
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

#include <config.h>
#include <string.h>

#include "libvirt-sandbox/libvirt-sandbox.h"

#define GVIR_SANDBOX_BUILDER_CONTAINER_GET_PRIVATE(obj)                         \
        (G_TYPE_INSTANCE_GET_PRIVATE((obj), GVIR_SANDBOX_TYPE_BUILDER_CONTAINER, GVirSandboxBuilderContainerPrivate))

struct _GVirSandboxBuilderContainerPrivate
{
    gboolean unused;
};

G_DEFINE_TYPE(GVirSandboxBuilderContainer, gvir_sandbox_builder_container, GVIR_SANDBOX_TYPE_BUILDER);


enum {
    PROP_0,
};

enum {
    LAST_SIGNAL
};

//static gint signals[LAST_SIGNAL];

#define GVIR_SANDBOX_BUILDER_CONTAINER_ERROR gvir_sandbox_builder_container_error_quark()

static GQuark
gvir_sandbox_builder_container_error_quark(void)
{
    return g_quark_from_static_string("gvir-sandbox-builder-container");
}


static void gvir_sandbox_builder_container_get_property(GObject *object,
                                                      guint prop_id,
                                                      GValue *value G_GNUC_UNUSED,
                                                      GParamSpec *pspec)
{
#if 0
    GVirSandboxBuilderContainer *builder = GVIR_SANDBOX_BUILDER_CONTAINER(object);
    GVirSandboxBuilderContainerPrivate *priv = builder->priv;
#endif

    switch (prop_id) {

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_builder_container_set_property(GObject *object,
                                                      guint prop_id,
                                                      const GValue *value G_GNUC_UNUSED,
                                                      GParamSpec *pspec)
{
#if 0
    GVirSandboxBuilderContainer *builder = GVIR_SANDBOX_BUILDER_CONTAINER(object);
    GVirSandboxBuilderContainerPrivate *priv = builder->priv;
#endif

    switch (prop_id) {

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_builder_container_finalize(GObject *object)
{
#if 0
    GVirSandboxBuilderContainer *builder = GVIR_SANDBOX_BUILDER_CONTAINER(object);
    GVirSandboxBuilderContainerPrivate *priv = builder->priv;
#endif

    G_OBJECT_CLASS(gvir_sandbox_builder_container_parent_class)->finalize(object);
}


static gchar *gvir_sandbox_builder_container_cmdline(GVirSandboxConfig *config)
{
    GString *str = g_string_new("");
    gchar **argv, **tmp;
    gchar *cmdrawstr;
    gchar *cmdb64str;
    size_t len = 0, offset;
    gchar *ret;

    argv = gvir_sandbox_config_get_command(config);

    tmp = argv;
    while (*tmp) {
        len += strlen(*tmp) + 1;
        tmp++;
    }

    cmdrawstr = g_new0(gchar, len);
    tmp = argv;
    offset = 0;
    while (*tmp) {
        size_t tlen = strlen(*tmp);
        memcpy(cmdrawstr + offset, *tmp, tlen + 1);
        offset += tlen + 1;
        tmp++;
    }

    cmdb64str = g_base64_encode((guchar*)cmdrawstr, len);
    g_free(cmdrawstr);

    /* First sandbox specific args */
    g_string_append_printf(str, "sandbox-cmd=%s", cmdb64str);
    g_free(cmdb64str);

    if (gvir_sandbox_config_get_tty(config))
        g_string_append(str, " sandbox-isatty");

    g_string_append_printf(str, " sandbox-uid=%u", gvir_sandbox_config_get_userid(config));
    g_string_append_printf(str, " sandbox-gid=%u", gvir_sandbox_config_get_groupid(config));
    g_string_append_printf(str, " sandbox-user=%s", gvir_sandbox_config_get_username(config));
    g_string_append_printf(str, " sandbox-home=%s", gvir_sandbox_config_get_homedir(config));

    ret = str->str;
    g_string_free(str, FALSE);
    return ret;
}



static GVirConfigDomain *gvir_sandbox_builder_container_construct(GVirSandboxBuilder *builder G_GNUC_UNUSED,
                                                                  GVirSandboxConfig *config,
                                                                  GVirSandboxCleaner *cleaner G_GNUC_UNUSED,
                                                                  GError **error G_GNUC_UNUSED)
{
    GString *str = g_string_new("<domain type='lxc'>\n");
    GVirConfigDomain *dom = NULL;
    gchar *cmdline = NULL;
    GList *mounts = NULL, *tmp = NULL;

    cmdline = gvir_sandbox_builder_container_cmdline(config);

    g_string_append_printf(str, "  <name>%s</name>\n",
                           gvir_sandbox_config_get_name(config));
    g_string_append_printf(str, "  <memory>%d</memory>\n", 512*1024);

    g_string_append(str, "  <os>\n");
    g_string_append_printf(str, "    <type arch='%s'>exe</type>\n",
                           gvir_sandbox_config_get_arch(config));
    g_string_append_printf(str, "    <init>%s</init>\n", LIBEXECDIR "/libvirt-sandbox-init-lxc");
    g_string_append_printf(str, "    <cmdline>%s</cmdline>\n", cmdline);
    g_string_append(str, "  </os>\n");

    g_string_append(str, "  <devices>\n");
    g_string_append(str, "    <console type='pty'/>\n");

    g_string_append(str, "    <filesystem type='mount' accessmode='passthrough'>\n");
    g_string_append_printf(str, "      <source dir='%s'/>\n",
                           gvir_sandbox_config_get_root(config));
    g_string_append(str, "      <target dir='/'/>\n");
    g_string_append(str, "      <readonly/>\n");
    g_string_append(str, "    </filesystem>\n");

    tmp = mounts = gvir_sandbox_config_get_mounts(config);
    while (tmp) {
        GVirSandboxConfigMount *mconfig = tmp->data;

        g_string_append(str, "    <filesystem type='mount' accessmode='passthrough'>\n");
        g_string_append_printf(str, "      <source dir='%s'/>\n",
                               gvir_sandbox_config_mount_get_root(mconfig));
        g_string_append_printf(str, "      <target dir='%s'/>\n",
                               gvir_sandbox_config_mount_get_target(mconfig));
        g_string_append(str, "      <readonly/>\n");
        g_string_append(str, "    </filesystem>\n");

        tmp = tmp->next;
    }
    g_list_foreach(mounts, (GFunc)g_object_unref, NULL);
    g_list_free(mounts);
    mounts = NULL;

    if (GVIR_SANDBOX_IS_CONFIG_GRAPHICAL(config)) {
        *error = g_error_new(GVIR_SANDBOX_BUILDER_CONTAINER_ERROR, 0,
                             "%s", "Graphical sandboxes are not supported for containers");
        goto cleanup;
    }

    g_string_append(str, "  </devices>\n");

    if (gvir_sandbox_config_get_security_level(config)) {
        g_string_append(str, "  <seclabel type='static' model='selinux'>\n");
        g_string_append_printf(str, "    <label>system_u:system_r:%s:s0:%s</label>\n",
                               gvir_sandbox_config_get_security_type(config),
                               gvir_sandbox_config_get_security_level(config));
        g_string_append(str, "  </seclabel>\n");
    } else {
        g_string_append(str, "  <seclabel type='dynamic' model='selinux'>\n");
        g_string_append_printf(str, "    <baselabel>system_u:system_r:%s:s0</baselabel>\n",
                               gvir_sandbox_config_get_security_type(config));
        g_string_append(str, "  </seclabel>\n");
    }


    g_string_append(str, "</domain>");

    dom = gvir_config_domain_new_from_xml(str->str, error);

cleanup:
    g_string_free(str, TRUE);
    g_free(cmdline);
    return dom;
}


static void gvir_sandbox_builder_container_class_init(GVirSandboxBuilderContainerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    GVirSandboxBuilderClass *builder_class = GVIR_SANDBOX_BUILDER_CLASS(klass);

    object_class->finalize = gvir_sandbox_builder_container_finalize;
    object_class->get_property = gvir_sandbox_builder_container_get_property;
    object_class->set_property = gvir_sandbox_builder_container_set_property;

    builder_class->construct = gvir_sandbox_builder_container_construct;

    g_type_class_add_private(klass, sizeof(GVirSandboxBuilderContainerPrivate));
}


static void gvir_sandbox_builder_container_init(GVirSandboxBuilderContainer *builder)
{
    builder->priv = GVIR_SANDBOX_BUILDER_CONTAINER_GET_PRIVATE(builder);
}


/**
 * gvir_sandbox_builder_container_new:
 * @connection: (transfer none): the connection
 *
 * Create a new graphical application sandbox builderuration
 *
 * Returns: (transfer full): a new graphical sandbox builder object
 */
GVirSandboxBuilderContainer *gvir_sandbox_builder_container_new(GVirConnection *connection)
{
    return GVIR_SANDBOX_BUILDER_CONTAINER(g_object_new(GVIR_SANDBOX_TYPE_BUILDER_CONTAINER,
                                                       "connection", connection,
                                                       NULL));
}
