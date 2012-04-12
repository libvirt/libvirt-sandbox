/*
 * libvirt-sandbox-console-raw.c: libvirt sandbox raw console
 *
 * Copyright (C) 2011-2012 Red Hat, Inc.
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

#include <gio/gfiledescriptorbased.h>
#include <termios.h>
#include <errno.h>
#include <string.h>
#include <libvirt-glib/libvirt-glib-error.h>

#include "libvirt-sandbox/libvirt-sandbox.h"

/**
 * SECTION: libvirt-sandbox-console-raw
 * @short_description: A text mode raw console
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

#define GVIR_SANDBOX_CONSOLE_RAW_GET_PRIVATE(obj)                       \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), GVIR_SANDBOX_TYPE_CONSOLE_RAW, GVirSandboxConsoleRawPrivate))

struct _GVirSandboxConsoleRawPrivate
{
    GVirConnection *connection;
    GVirDomain *domain;

    GVirStream *console;
    gchar *devname;

    GUnixInputStream *localStdin;
    GUnixOutputStream *localStdout;
    GUnixOutputStream *localStderr;

    gboolean termiosActive;
    struct termios termiosProps;

    gchar *consoleToLocal;
    gsize consoleToLocalLength;
    gsize consoleToLocalOffset;
    gchar *localToConsole;
    gsize localToConsoleLength;
    gsize localToConsoleOffset;

    /* True if stdin has shown us EOF */
    gboolean localEOF;

    GSource *localStdinSource;
    GSource *localStdoutSource;
    GSource *localStderrSource;
    gint consoleWatch;
};

static void gvir_sandbox_console_raw_interface_init(gpointer g_iface,
                                                    gpointer iface_data);

G_DEFINE_TYPE_EXTENDED(GVirSandboxConsoleRaw, gvir_sandbox_console_raw, G_TYPE_OBJECT, 0,
                       G_IMPLEMENT_INTERFACE(GVIR_SANDBOX_TYPE_CONSOLE,
                                             gvir_sandbox_console_raw_interface_init));


enum {
    PROP_0,

    PROP_CONNECTION,
    PROP_DOMAIN,
    PROP_DEVNAME,
};

enum {
    LAST_SIGNAL
};

//static gint signals[LAST_SIGNAL];

#define GVIR_SANDBOX_CONSOLE_RAW_ERROR gvir_sandbox_console_raw_error_quark()

static GQuark
gvir_sandbox_console_raw_error_quark(void)
{
    return g_quark_from_static_string("gvir-sandbox-console-raw");
}

static void gvir_sandbox_console_raw_get_property(GObject *object,
                                                  guint prop_id,
                                                  GValue *value,
                                                  GParamSpec *pspec)
{
    GVirSandboxConsoleRaw *console = GVIR_SANDBOX_CONSOLE_RAW(object);
    GVirSandboxConsoleRawPrivate *priv = console->priv;

    switch (prop_id) {
    case PROP_CONNECTION:
        g_value_set_object(value, priv->connection);
        break;

    case PROP_DOMAIN:
        g_value_set_object(value, priv->domain);
        break;

    case PROP_DEVNAME:
        g_value_set_string(value, priv->devname);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_console_raw_set_property(GObject *object,
                                                  guint prop_id,
                                                  const GValue *value,
                                                  GParamSpec *pspec)
{
    GVirSandboxConsoleRaw *console = GVIR_SANDBOX_CONSOLE_RAW(object);
    GVirSandboxConsoleRawPrivate *priv = console->priv;

    switch (prop_id) {
    case PROP_CONNECTION:
        if (priv->connection)
            g_object_unref(priv->connection);
        priv->connection = g_value_dup_object(value);
        break;

    case PROP_DOMAIN:
        if (priv->domain)
            g_object_unref(priv->domain);
        priv->domain = g_value_dup_object(value);
        break;

    case PROP_DEVNAME:
        priv->devname = g_value_dup_string(value);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_console_raw_finalize(GObject *object)
{
    GVirSandboxConsoleRaw *console = GVIR_SANDBOX_CONSOLE_RAW(object);
    GVirSandboxConsoleRawPrivate *priv = console->priv;

    if (priv->localStdin)
        gvir_sandbox_console_detach(GVIR_SANDBOX_CONSOLE(console), NULL);

    /* All other private fields are free'd by the detach call */

    if (priv->domain)
        g_object_unref(priv->domain);
    if (priv->connection)
        g_object_unref(priv->connection);

    g_free(priv->devname);

    G_OBJECT_CLASS(gvir_sandbox_console_raw_parent_class)->finalize(object);
}


static void gvir_sandbox_console_raw_class_init(GVirSandboxConsoleRawClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = gvir_sandbox_console_raw_finalize;
    object_class->get_property = gvir_sandbox_console_raw_get_property;
    object_class->set_property = gvir_sandbox_console_raw_set_property;

    g_object_class_install_property(object_class,
                                    PROP_CONNECTION,
                                    g_param_spec_object("connection",
                                                        "Connection",
                                                        "The sandbox connection",
                                                        GVIR_TYPE_CONNECTION,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_DOMAIN,
                                    g_param_spec_object("domain",
                                                        "Domain",
                                                        "The sandbox domain",
                                                        GVIR_TYPE_DOMAIN,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_DEVNAME,
                                    g_param_spec_string("devname",
                                                        "Devicename",
                                                        "Device name",
                                                        NULL,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_signal_new("closed",
                 G_OBJECT_CLASS_TYPE(object_class),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(GVirSandboxConsoleRawClass, closed),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__BOOLEAN,
                 G_TYPE_NONE,
                 1,
                 G_TYPE_BOOLEAN);

    g_type_class_add_private(klass, sizeof(GVirSandboxConsoleRawPrivate));
}


static void gvir_sandbox_console_raw_init(GVirSandboxConsoleRaw *console)
{
    console->priv = GVIR_SANDBOX_CONSOLE_RAW_GET_PRIVATE(console);
}


/**
 * gvir_sandbox_console_raw_new:
 * @connection: (transfer none): the libvirt connection
 * @domain: (transfer none): the libvirt domain whose console_raw to run
 * @devname: the console to connect to
 *
 * Create a new sandbox raw console from the specified configuration
 *
 * Returns: (transfer full): a new sandbox console object
 */
GVirSandboxConsoleRaw *gvir_sandbox_console_raw_new(GVirConnection *connection,
                                                    GVirDomain *domain,
                                                    const char *devname)
{
    return GVIR_SANDBOX_CONSOLE_RAW(g_object_new(GVIR_SANDBOX_TYPE_CONSOLE_RAW,
                                                 "connection", connection,
                                                 "domain", domain,
                                                 "devname", devname,
                                                 NULL));
}


static gboolean gvir_sandbox_console_raw_start_term(GVirSandboxConsoleRaw *console,
                                                    GUnixInputStream *localStdin,
                                                    GError **error)
{
    GVirSandboxConsoleRawPrivate *priv = console->priv;
    int fd = g_unix_input_stream_get_fd(localStdin);
    struct termios ios;

    if (!isatty(fd))
        return TRUE;

    if (tcgetattr(fd, &priv->termiosProps) < 0) {
        g_set_error(error, GVIR_SANDBOX_CONSOLE_RAW_ERROR, 0,
                    "Unable to query terminal attributes: %s",
                    strerror(errno));
        return FALSE;
    }

    memcpy(&ios, &priv->termiosProps, sizeof(ios));

    /* Put into raw mode */
    ios.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP |
                     INLCR | IGNCR | ICRNL | IXON);
    ios.c_oflag &= ~(OPOST);
    ios.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    ios.c_cflag &= ~(CSIZE |PARENB);
    ios.c_cflag |= CS8;

    if (tcsetattr(fd, TCSADRAIN, &ios) < 0) {
        g_set_error(error, GVIR_SANDBOX_CONSOLE_RAW_ERROR, 0,
                    "Unable to update terminal attributes: %s",
                    strerror(errno));
        return FALSE;
    }

    priv->termiosActive = TRUE;

    return TRUE;
}


static gboolean gvir_sandbox_console_raw_stop_term(GVirSandboxConsoleRaw *console,
                                                   GUnixInputStream *localStdin,
                                                   GError **error)
{
    GVirSandboxConsoleRawPrivate *priv = console->priv;
    int fd = g_unix_input_stream_get_fd(localStdin);

    if (!isatty(fd))
        return TRUE;

    if (tcsetattr(fd, TCSADRAIN, &priv->termiosProps) < 0) {
        g_set_error(error, GVIR_SANDBOX_CONSOLE_RAW_ERROR, 0,
                    "Unable to restore terminal attributes: %s",
                    strerror(errno));
        return FALSE;
    }

    priv->termiosActive = FALSE;

    return TRUE;
}


static gboolean do_console_raw_stream_readwrite(GVirStream *stream,
                                                GVirStreamIOCondition cond,
                                                gpointer opaque);
static gboolean do_console_raw_local_read(GObject *stream,
                                          gpointer opaque);
static gboolean do_console_raw_local_write(GObject *stream,
                                           gpointer opaque);

static void do_console_raw_update_events(GVirSandboxConsoleRaw *console)
{
    GVirSandboxConsoleRawPrivate *priv = console->priv;
    GVirStreamIOCondition cond = 0;

    if (!priv->console) /* Closed */
        return;

    if (priv->localStdin) {
        if ((priv->localToConsoleOffset < priv->localToConsoleLength) &&
            (!priv->localEOF)) {
            if (priv->localStdinSource == NULL) {
                priv->localStdinSource = g_pollable_input_stream_create_source
                    (G_POLLABLE_INPUT_STREAM(priv->localStdin), NULL);
                g_source_set_callback(priv->localStdinSource,
                                      (GSourceFunc)do_console_raw_local_read,
                                      console,
                                      NULL);
                g_source_attach(priv->localStdinSource,
                                g_main_context_default());
            }
        } else {
            if (priv->localStdinSource) {
                g_source_destroy(priv->localStdinSource);
                g_source_unref(priv->localStdinSource);
                priv->localStdinSource = NULL;
            }
        }
    }

    /*
     * With raw consoles we can't distinguish stdout/stderr, so everything
     * goes to stdout
     */
    if (priv->localStdout) {
        if (priv->consoleToLocalOffset) {
            if (priv->localStdoutSource == NULL) {
                priv->localStdoutSource = g_pollable_output_stream_create_source
                    (G_POLLABLE_OUTPUT_STREAM(priv->localStdout), NULL);
                g_source_set_callback(priv->localStdoutSource,
                                      (GSourceFunc)do_console_raw_local_write,
                                      console,
                                      NULL);
                g_source_attach(priv->localStdoutSource,
                                g_main_context_default());
            }
        } else {
            if (priv->localStdoutSource) {
                g_source_destroy(priv->localStdoutSource);
                g_source_unref(priv->localStdoutSource);
                priv->localStdoutSource = NULL;
            }
        }
    } else {
        if (priv->consoleToLocalOffset) {
            if (priv->localStderrSource == NULL) {
                priv->localStderrSource = g_pollable_output_stream_create_source
                    (G_POLLABLE_OUTPUT_STREAM(priv->localStderr), NULL);
                g_source_set_callback(priv->localStderrSource,
                                      (GSourceFunc)do_console_raw_local_write,
                                      console,
                                      NULL);
                g_source_attach(priv->localStderrSource,
                                g_main_context_default());
            }
        } else {
            if (priv->localStderrSource) {
                g_source_destroy(priv->localStderrSource);
                g_source_unref(priv->localStderrSource);
                priv->localStderrSource = NULL;
            }
        }
    }

    if (priv->localToConsoleOffset)
        cond |= GVIR_STREAM_IO_CONDITION_WRITABLE;
    if (priv->consoleToLocalOffset < priv->consoleToLocalLength)
        cond |= GVIR_STREAM_IO_CONDITION_READABLE;

    if (priv->consoleWatch) {
        g_source_remove(priv->consoleWatch);
        priv->consoleWatch = 0;
    }
    if (cond) {
        priv->consoleWatch = gvir_stream_add_watch(priv->console,
                                                   cond,
                                                   do_console_raw_stream_readwrite,
                                                   console);
    }

}

static void do_console_raw_close(GVirSandboxConsoleRaw *console,
                                 GError *error)
{
    gvir_sandbox_console_detach(GVIR_SANDBOX_CONSOLE(console), NULL);
    g_signal_emit_by_name(console, "closed", error != NULL);
}

static gboolean do_console_raw_stream_readwrite(GVirStream *stream,
                                                GVirStreamIOCondition cond,
                                                gpointer opaque)
{
    GVirSandboxConsoleRaw *console = GVIR_SANDBOX_CONSOLE_RAW(opaque);
    GVirSandboxConsoleRawPrivate *priv = console->priv;

    if (cond & GVIR_STREAM_IO_CONDITION_READABLE) {
        GError *err = NULL;
        gssize ret = gvir_stream_receive
            (stream,
             priv->consoleToLocal + priv->consoleToLocalOffset,
             priv->consoleToLocalLength - priv->consoleToLocalOffset,
             NULL,
             &err);
        if (ret < 0) {
            if (err && err->code == G_IO_ERROR_WOULD_BLOCK) {
                /* Shouldn't get this, but you never know */
                g_error_free(err);
                goto done;
            } else {
                g_debug("Error from stream recv %s", err ? err->message : "");
                do_console_raw_close(console, err);
                g_error_free(err);
                goto cleanup;
            }
        }
        if (ret == 0) { /* EOF */
            goto done;
        }
        priv->consoleToLocalOffset += ret;
    }

    if (cond & GVIR_STREAM_IO_CONDITION_WRITABLE) {
        GError *err = NULL;
        gssize ret = gvir_stream_send(stream,
                                      priv->localToConsole,
                                      priv->localToConsoleOffset,
                                      NULL,
                                      &err);
        if (ret < 0) {
            g_debug("Error from stream send %s", err ? err->message : "");
            do_console_raw_close(console, err);
            g_error_free(err);
            goto cleanup;
        }

        if (priv->localToConsoleOffset != ret)
            memmove(priv->localToConsole,
                    priv->localToConsole + ret,
                    priv->localToConsoleOffset - ret);
        priv->localToConsoleOffset -= ret;
    }

done:
    priv->consoleWatch = 0;
    do_console_raw_update_events(console);

cleanup:
    return FALSE;
}

static gboolean do_console_raw_local_read(GObject *stream,
                                          gpointer opaque)
{
    GUnixInputStream *localStdin = G_UNIX_INPUT_STREAM(stream);
    GVirSandboxConsoleRaw *console = GVIR_SANDBOX_CONSOLE_RAW(opaque);
    GVirSandboxConsoleRawPrivate *priv = console->priv;
    GError *err = NULL;

    gssize ret = g_pollable_input_stream_read_nonblocking
        (G_POLLABLE_INPUT_STREAM(localStdin),
         priv->localToConsole + priv->localToConsoleOffset,
         priv->localToConsoleLength - priv->localToConsoleOffset,
         NULL, &err);
    if (ret < 0) {
        g_debug("Error from local read %s", err ? err->message : "");
        do_console_raw_close(console, err);
        g_error_free(err);
        goto cleanup;
    }

    if (ret == 0)
        priv->localEOF = TRUE;

    priv->localToConsoleOffset += ret;

    priv->localStdinSource = NULL;
    do_console_raw_update_events(console);
cleanup:
    return FALSE;
}


static gboolean do_console_raw_local_write(GObject *stream,
                                           gpointer opaque)
{
    GUnixOutputStream *localStdoutOrStderr = G_UNIX_OUTPUT_STREAM(stream);
    GVirSandboxConsoleRaw *console = GVIR_SANDBOX_CONSOLE_RAW(opaque);
    GVirSandboxConsoleRawPrivate *priv = console->priv;
    GError *err = NULL;

    gssize ret = g_pollable_output_stream_write_nonblocking
        (G_POLLABLE_OUTPUT_STREAM(localStdoutOrStderr),
         priv->consoleToLocal,
         priv->consoleToLocalOffset,
         NULL, &err);
    if (ret < 0) {
        g_debug("Error from local write %s", err ? err->message : "");
        do_console_raw_close(console, err);
        g_error_free(err);
        goto cleanup;
    }

    if (priv->consoleToLocalOffset != ret)
        memmove(priv->consoleToLocal,
                priv->consoleToLocal + ret,
                priv->consoleToLocalOffset - ret);
    priv->consoleToLocalOffset -= ret;

    priv->localStdoutSource = NULL;
    do_console_raw_update_events(console);
cleanup:
    return FALSE;
}


static gboolean gvir_sandbox_console_raw_attach(GVirSandboxConsole *console,
                                                GUnixInputStream *localStdin,
                                                GUnixOutputStream *localStdout,
                                                GUnixOutputStream *localStderr,
                                                GError **error)
{
    GVirSandboxConsoleRawPrivate *priv = GVIR_SANDBOX_CONSOLE_RAW(console)->priv;
    gboolean ret = FALSE;

    if (priv->console) {
        g_set_error(error, GVIR_SANDBOX_CONSOLE_RAW_ERROR, 0, "%s",
                    "Console is already attached to a stream");
        return FALSE;
    }

    if (localStdin &&
        !gvir_sandbox_console_raw_start_term(GVIR_SANDBOX_CONSOLE_RAW(console),
                                             localStdin, error))
        return FALSE;

    priv->localStdin = localStdin ? g_object_ref(localStdin) : NULL;
    priv->localStdout = localStdout ? g_object_ref(localStdout) : NULL;
    priv->localStderr = g_object_ref(localStderr);

    priv->console = gvir_connection_get_stream(priv->connection, 0);

    if (!gvir_domain_open_console(priv->domain, priv->console,
                                  priv->devname, 0, error))
        goto cleanup;

    priv->consoleToLocalLength = 4096;
    priv->consoleToLocal = g_new0(gchar, priv->consoleToLocalLength);
    if (localStdin) {
        priv->localToConsoleLength = 4096;
        priv->localToConsole = g_new0(gchar, priv->localToConsoleLength);
    }

    do_console_raw_update_events(GVIR_SANDBOX_CONSOLE_RAW(console));

    ret = TRUE;
cleanup:
    if (!ret && localStdin)
        gvir_sandbox_console_raw_stop_term(GVIR_SANDBOX_CONSOLE_RAW(console),
                                           localStdin, NULL);
    return ret;
}


static gboolean gvir_sandbox_console_raw_detach(GVirSandboxConsole *console,
                                                GError **error)
{
    GVirSandboxConsoleRawPrivate *priv = GVIR_SANDBOX_CONSOLE_RAW(console)->priv;
    gboolean ret = FALSE;
    if (!priv->console) {
        return TRUE;
#if 0
        g_set_error(error, GVIR_SANDBOX_CONSOLE_RAW_ERROR, 0, "%s",
                    "Console is not attached to a stream");
        return FALSE;
#endif
    }

    if (priv->localStdin &&
        !gvir_sandbox_console_raw_stop_term(GVIR_SANDBOX_CONSOLE_RAW(console),
                                            priv->localStdin, error))
        return FALSE;

    if (priv->console) {
        g_object_unref(priv->console);
        priv->console = NULL;
        priv->consoleToLocalOffset = priv->consoleToLocalLength = 0;
        priv->localToConsoleOffset = priv->localToConsoleLength = 0;
        g_free(priv->consoleToLocal);
        g_free(priv->localToConsole);
        priv->consoleToLocal = priv->localToConsole = NULL;
    }

    if (priv->localStdinSource)
        g_source_unref(priv->localStdinSource);
    if (priv->localStdoutSource)
        g_source_unref(priv->localStdoutSource);
    if (priv->localStderrSource)
        g_source_unref(priv->localStderrSource);
    if (priv->consoleWatch)
        g_source_remove(priv->consoleWatch);
    priv->localStdinSource = priv->localStdoutSource = priv->localStderrSource = NULL;
    priv->consoleWatch = 0;

    if (priv->localStdin)
        g_object_unref(priv->localStdin);
    if (priv->localStdout)
        g_object_unref(priv->localStdout);
    g_object_unref(priv->localStderr);
    priv->localStdin = NULL;
    priv->localStdout = NULL;
    priv->localStderr = NULL;

    ret = TRUE;

//cleanup:
    return ret;
}


static void gvir_sandbox_console_raw_interface_init(gpointer g_iface,
                                                    gpointer iface_data)
{
    GVirSandboxConsoleInterface *iface = g_iface;
    iface->attach = gvir_sandbox_console_raw_attach;
    iface->detach = gvir_sandbox_console_raw_detach;
}
