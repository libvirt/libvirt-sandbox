/*
 * libvirt-sandbox-builder-machine.c: libvirt sandbox configuration
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

#define GVIR_SANDBOX_BUILDER_MACHINE_GET_PRIVATE(obj)                         \
        (G_TYPE_INSTANCE_GET_PRIVATE((obj), GVIR_SANDBOX_TYPE_BUILDER_MACHINE, GVirSandboxBuilderMachinePrivate))

struct _GVirSandboxBuilderMachinePrivate
{
    gboolean unused;
};

G_DEFINE_TYPE(GVirSandboxBuilderMachine, gvir_sandbox_builder_machine, GVIR_SANDBOX_TYPE_BUILDER);


enum {
    PROP_0,
};

enum {
    LAST_SIGNAL
};

//static gint signals[LAST_SIGNAL];

#define GVIR_SANDBOX_BUILDER_MACHINE_ERROR gvir_sandbox_builder_machine_error_quark()

static GQuark
gvir_sandbox_builder_machine_error_quark(void)
{
    return g_quark_from_static_string("gvir-sandbox-builder-machine");
}


static void gvir_sandbox_builder_machine_get_property(GObject *object,
                                                      guint prop_id,
                                                      GValue *value G_GNUC_UNUSED,
                                                      GParamSpec *pspec)
{
#if 0
    GVirSandboxBuilderMachine *builder = GVIR_SANDBOX_BUILDER_MACHINE(object);
    GVirSandboxBuilderMachinePrivate *priv = builder->priv;
#endif

    switch (prop_id) {

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_builder_machine_set_property(GObject *object,
                                                      guint prop_id,
                                                      const GValue *value G_GNUC_UNUSED,
                                                      GParamSpec *pspec)
{
#if 0
    GVirSandboxBuilderMachine *builder = GVIR_SANDBOX_BUILDER_MACHINE(object);
    GVirSandboxBuilderMachinePrivate *priv = builder->priv;
#endif

    switch (prop_id) {

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_builder_machine_finalize(GObject *object)
{
#if 0
    GVirSandboxBuilderMachine *builder = GVIR_SANDBOX_BUILDER_MACHINE(object);
    GVirSandboxBuilderMachinePrivate *priv = builder->priv;
#endif

    G_OBJECT_CLASS(gvir_sandbox_builder_machine_parent_class)->finalize(object);
}


static GVirConfigDomain *gvir_sandbox_builder_machine_construct(GVirSandboxBuilder *builder G_GNUC_UNUSED,
                                                                GVirSandboxConfig *config,
                                                                GVirSandboxCleaner *cleaner G_GNUC_UNUSED,
                                                                GError **error)
{
    GString *str = g_string_new("<domain type='kvm'>\n");
    GVirConfigDomain *dom;
    const gchar *kernel = "";
    const gchar *initrd = "";
    const gchar *cmdline = "";
    GList *mounts = NULL, *tmp = NULL;

    g_string_append_printf(str, "  <name>%s</name>\n",
                           gvir_sandbox_config_get_name(config));
    g_string_append_printf(str, "  <memory>%d</memory>\n", 512*1024);

    g_string_append(str, "  <os>\n");
    g_string_append_printf(str, "    <type arch='%s'>hvm</type>\n",
                           gvir_sandbox_config_get_arch(config));
    g_string_append_printf(str, "    <kernel>%s</kernel>\n", kernel);
    g_string_append_printf(str, "    <initrd>%s</initrd>\n", initrd);
    g_string_append_printf(str, "    <cmdline>%s</cmdline>\n", cmdline);
    g_string_append(str, "  </os>\n");

    g_string_append(str, "  <features>\n");
    g_string_append(str, "    <acpi/>\n");
    g_string_append(str, "  </features>\n");

    g_string_append(str, "  <devices>\n");
    g_string_append(str, "    <console type='pty'/>\n");

    g_string_append(str, "    <filesystem type='mount' accessmode='passthrough'>\n");
    g_string_append_printf(str, "      <source dir='%s'/>\n",
                           gvir_sandbox_config_get_root(config));
    g_string_append(str, "      <target dir='sandbox:root'/>\n");
    g_string_append(str, "      <readonly/>\n");
    g_string_append(str, "    </filesystem>\n");

    tmp = mounts = gvir_sandbox_config_get_mounts(config);
    while (tmp) {
        GVirSandboxConfigMount *mconfig = tmp->data;
        const gchar *home = getenv("HOME");
        const gchar *tgtsym;
        const gchar *target = gvir_sandbox_config_mount_get_target(mconfig);
        if (home &&
            (strcmp(target, home) == 0))
            tgtsym = "sandbox:home";
        else if (strcmp(target, "/tmp") == 0)
            tgtsym = "sandbox:tmp";
        else {
            *error = g_error_new(GVIR_SANDBOX_BUILDER_MACHINE_ERROR, 0,
                                 "Cannot export mount to %s", target);
            goto error;
        }


        g_string_append(str, "    <filesystem type='mount' accessmode='passthrough'>\n");
        g_string_append_printf(str, "      <source dir='%s'/>\n",
                               gvir_sandbox_config_mount_get_root(mconfig));
        g_string_append_printf(str, "      <target dir='%s'/>\n", tgtsym);
        g_string_append(str, "      <readonly/>\n");
        g_string_append(str, "    </filesystem>\n");

        tmp = tmp->next;
    }
    g_list_foreach(mounts, (GFunc)g_object_unref, NULL);
    g_list_free(mounts);
    mounts = NULL;

    g_string_append(str, "    <memballoon model='none'/>\n");

    if (GVIR_SANDBOX_IS_CONFIG_GRAPHICAL(config)) {
        const gchar *xauth = getenv("XAUTHORITY");

        if (xauth)
            g_string_append_printf(str, "    <graphics type='sdl' display=':0' xauth='%s'/>\n", xauth);
        else
            g_string_append(str, "    <graphics type='sdl' display=':0'/>\n");
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
    g_string_free(str, TRUE);
    return dom;

error:
    g_list_foreach(mounts, (GFunc)g_object_unref, NULL);
    g_list_free(mounts);
    g_string_free(str, TRUE);
    return NULL;
}


static void gvir_sandbox_builder_machine_class_init(GVirSandboxBuilderMachineClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    GVirSandboxBuilderClass *builder_class = GVIR_SANDBOX_BUILDER_CLASS(klass);

    object_class->finalize = gvir_sandbox_builder_machine_finalize;
    object_class->get_property = gvir_sandbox_builder_machine_get_property;
    object_class->set_property = gvir_sandbox_builder_machine_set_property;

    builder_class->construct = gvir_sandbox_builder_machine_construct;

    g_type_class_add_private(klass, sizeof(GVirSandboxBuilderMachinePrivate));
}


static void gvir_sandbox_builder_machine_init(GVirSandboxBuilderMachine *builder)
{
    builder->priv = GVIR_SANDBOX_BUILDER_MACHINE_GET_PRIVATE(builder);
}


/**
 * gvir_sandbox_builder_machine_new:
 * @connection: (transfer none): the connection
 *
 * Create a new graphical application sandbox builderuration
 *
 * Returns: (transfer full): a new graphical sandbox builder object
 */
GVirSandboxBuilderMachine *gvir_sandbox_builder_machine_new(GVirConnection *connection)
{
    return GVIR_SANDBOX_BUILDER_MACHINE(g_object_new(GVIR_SANDBOX_TYPE_BUILDER_MACHINE,
                                                     "connection", connection,
                                                     NULL));
}
