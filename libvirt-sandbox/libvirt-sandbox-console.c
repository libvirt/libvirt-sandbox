/*
 * libvirt-sandbox-console.c: libvirt sandbox console
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

#include <gio/gfiledescriptorbased.h>
#include <termios.h>
#include <errno.h>
#include <string.h>
#include <libvirt/libvirt.h>
#include <libvirt-glib/libvirt-glib-error.h>

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

#define GVIR_SANDBOX_CONSOLE_GET_PRIVATE(obj)                         \
        (G_TYPE_INSTANCE_GET_PRIVATE((obj), GVIR_SANDBOX_TYPE_CONSOLE, GVirSandboxConsolePrivate))

/* This magic string comes to you all the way from the southern hemisphere of the world */
#define GVIR_SANDBOX_HANDSHAKE_MAGIC "xoqpuɐs"

struct _GVirSandboxConsolePrivate
{
    GVirConnection *connection;
    GVirDomain *domain;

    GVirStream *console;

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
    /* True if guest handshake has completed */
    gboolean guestHandshake;
    /* True if handshake bytes didn't match */
    gboolean guestError;

    GSource *localStdinSource;
    GSource *localStdoutSource;
    GSource *localStderrSource;
    gint consoleWatch;
};

G_DEFINE_TYPE(GVirSandboxConsole, gvir_sandbox_console, G_TYPE_OBJECT);


enum {
    PROP_0,

    PROP_CONNECTION,
    PROP_DOMAIN,
};

enum {
    LAST_SIGNAL
};

//static gint signals[LAST_SIGNAL];

#define GVIR_SANDBOX_CONSOLE_ERROR gvir_sandbox_console_error_quark()

static GQuark
gvir_sandbox_console_error_quark(void)
{
    return g_quark_from_static_string("gvir-sandbox-console");
}

static void gvir_sandbox_console_get_property(GObject *object,
                                             guint prop_id,
                                             GValue *value,
                                             GParamSpec *pspec)
{
    GVirSandboxConsole *console = GVIR_SANDBOX_CONSOLE(object);
    GVirSandboxConsolePrivate *priv = console->priv;

    switch (prop_id) {
    case PROP_CONNECTION:
        g_value_set_object(value, priv->connection);
        break;

    case PROP_DOMAIN:
        g_value_set_object(value, priv->domain);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_console_set_property(GObject *object,
                                             guint prop_id,
                                             const GValue *value,
                                             GParamSpec *pspec)
{
    GVirSandboxConsole *console = GVIR_SANDBOX_CONSOLE(object);
    GVirSandboxConsolePrivate *priv = console->priv;

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

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_console_finalize(GObject *object)
{
    GVirSandboxConsole *console = GVIR_SANDBOX_CONSOLE(object);
    GVirSandboxConsolePrivate *priv = console->priv;

    if (priv->localStdin)
        gvir_sandbox_console_detach(console, NULL);

    /* All other private fields are free'd by the detach call */

    if (priv->domain)
        g_object_unref(priv->domain);
    if (priv->connection)
        g_object_unref(priv->connection);

    G_OBJECT_CLASS(gvir_sandbox_console_parent_class)->finalize(object);
}


static void gvir_sandbox_console_class_init(GVirSandboxConsoleClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = gvir_sandbox_console_finalize;
    object_class->get_property = gvir_sandbox_console_get_property;
    object_class->set_property = gvir_sandbox_console_set_property;

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

    g_signal_new("closed",
                 G_OBJECT_CLASS_TYPE(object_class),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(GVirSandboxConsoleClass, closed),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__BOOLEAN,
                 G_TYPE_NONE,
                 1,
                 G_TYPE_BOOLEAN);

    g_type_class_add_private(klass, sizeof(GVirSandboxConsolePrivate));
}


static void gvir_sandbox_console_init(GVirSandboxConsole *console)
{
    console->priv = GVIR_SANDBOX_CONSOLE_GET_PRIVATE(console);
}


/**
 * gvir_sandbox_console_new:
 * @connection: (transfer none): the libvirt connection
 * @domain: (transfer none): the libvirt domain whose console to run
 *
 * Create a new sandbox console from the specified configuration
 *
 * Returns: (transfer full): a new sandbox console object
 */
GVirSandboxConsole *gvir_sandbox_console_new(GVirConnection *connection,
                                             GVirDomain *domain)
{
    return GVIR_SANDBOX_CONSOLE(g_object_new(GVIR_SANDBOX_TYPE_CONSOLE,
                                             "connection", connection,
                                             "domain", domain,
                                             NULL));
}


static gboolean gvir_sandbox_console_start_term(GVirSandboxConsole *console,
                                                GUnixInputStream *localStdin,
                                                GError **error)
{
    GVirSandboxConsolePrivate *priv = console->priv;
    int fd = g_unix_input_stream_get_fd(localStdin);
    struct termios ios;

    if (!isatty(fd))
        return TRUE;

    if (tcgetattr(fd, &priv->termiosProps) < 0) {
        g_set_error(error, GVIR_SANDBOX_CONSOLE_ERROR, 0,
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
        g_set_error(error, GVIR_SANDBOX_CONSOLE_ERROR, 0,
                    "Unable to update terminal attributes: %s",
                    strerror(errno));
        return FALSE;
    }

    priv->termiosActive = TRUE;

    return TRUE;
}


static gboolean gvir_sandbox_console_stop_term(GVirSandboxConsole *console,
                                               GUnixInputStream *localStdin,
                                               GError **error)
{
    GVirSandboxConsolePrivate *priv = console->priv;
    int fd = g_unix_input_stream_get_fd(localStdin);

    if (!isatty(fd))
        return TRUE;

    if (tcsetattr(fd, TCSADRAIN, &priv->termiosProps) < 0) {
        g_set_error(error, GVIR_SANDBOX_CONSOLE_ERROR, 0,
                    "Unable to restore terminal attributes: %s",
                    strerror(errno));
        return FALSE;
    }

    priv->termiosActive = FALSE;

    return TRUE;
}


static gboolean do_console_stream_readwrite(GVirStream *stream,
                                            GVirStreamIOCondition cond,
                                            gpointer opaque);
static gboolean do_console_local_read(GObject *stream,
                                      gpointer opaque);
static gboolean do_console_local_write(GObject *stream,
                                       gpointer opaque);

static void do_console_update_events(GVirSandboxConsole *console)
{
    GVirSandboxConsolePrivate *priv = console->priv;
    GVirStreamIOCondition cond = 0;

    if (!priv->localStdin) /* Closed */
        return;

    /* We don't want to process local stdin, until we have the
     * handshake from the guest OS. THe reason is that when the
     * Linux kernel starts the console is *not* in raw mode.
     * Thus if we send any data to the guest, before the console
     * tty has been put into rawmode, it will be echoed before
     * even getting to the sandboxed application.
     */
    if ((priv->localToConsoleOffset < priv->localToConsoleLength) &&
        (!priv->localEOF) && priv->guestHandshake) {
        if (priv->localStdinSource == NULL) {
            priv->localStdinSource = g_pollable_input_stream_create_source
                (G_POLLABLE_INPUT_STREAM(priv->localStdin), NULL);
            g_source_set_callback(priv->localStdinSource,
                                  (GSourceFunc)do_console_local_read,
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

    /* If handshake has completed without error,
     * then all output should go to stdout
     */
    if (priv->consoleToLocalOffset && priv->guestHandshake && !priv->guestError) {
        if (priv->localStdoutSource == NULL) {
            priv->localStdoutSource = g_pollable_output_stream_create_source
                (G_POLLABLE_OUTPUT_STREAM(priv->localStdout), NULL);
            g_source_set_callback(priv->localStdoutSource,
                                  (GSourceFunc)do_console_local_write,
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

    /* If handshake has completed with error,
     * then all output should go to stderr
     */
    if (priv->consoleToLocalOffset && priv->guestHandshake && priv->guestError) {
        if (priv->localStderrSource == NULL) {
            priv->localStderrSource = g_pollable_output_stream_create_source
                (G_POLLABLE_OUTPUT_STREAM(priv->localStderr), NULL);
            g_source_set_callback(priv->localStderrSource,
                                  (GSourceFunc)do_console_local_write,
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
                                                   do_console_stream_readwrite,
                                                   console);
    }

}

static void do_console_close(GVirSandboxConsole *console,
                             GError *error)
{
    gvir_sandbox_console_detach(console, NULL);
    g_signal_emit_by_name(console, "closed", error != NULL);
}

static gboolean do_console_stream_readwrite(GVirStream *stream,
                                            GVirStreamIOCondition cond,
                                            gpointer opaque)
{
    GVirSandboxConsole *console = GVIR_SANDBOX_CONSOLE(opaque);
    GVirSandboxConsolePrivate *priv = console->priv;

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
                do_console_close(console, err);
                g_error_free(err);
                goto cleanup;
            }
        }
        if (ret == 0) { /* EOF */
            goto done;
        }
        priv->consoleToLocalOffset += ret;

        /*
         * The first thing we expect to see from the guest output is the
         * magic handshake byte sequence. If we don't see this, it indicates
         * that something has gone wrong in the startup process. If this
         * happens all output is forwarded to stderr, instead of stdout
         */
        if (!priv->guestHandshake &&
            (priv->consoleToLocalOffset >= sizeof(GVIR_SANDBOX_HANDSHAKE_MAGIC))) {
            priv->guestHandshake = TRUE;
            if (memcmp(priv->consoleToLocal,
                       GVIR_SANDBOX_HANDSHAKE_MAGIC,
                       sizeof(GVIR_SANDBOX_HANDSHAKE_MAGIC)) == 0) {
                /* Discard the matched handshake bytes */
                memmove(priv->consoleToLocal,
                        priv->consoleToLocal + sizeof(GVIR_SANDBOX_HANDSHAKE_MAGIC),
                        priv->consoleToLocalOffset - sizeof(GVIR_SANDBOX_HANDSHAKE_MAGIC));
                priv->consoleToLocalOffset -= sizeof(GVIR_SANDBOX_HANDSHAKE_MAGIC);
                /* Further output will go to stdout */
            } else {
                /* This causes output to now go to stderr */
                priv->guestError = TRUE;
            }
        }
    }

    if (cond & GVIR_STREAM_IO_CONDITION_WRITABLE) {
        GError *err = NULL;
        gssize ret = gvir_stream_send(stream,
                                      priv->localToConsole,
                                      priv->localToConsoleOffset,
                                      NULL,
                                      &err);
        if (ret < 0) {
            do_console_close(console, err);
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
    do_console_update_events(console);

cleanup:
    return FALSE;
}

static gboolean do_console_local_read(GObject *stream,
                                      gpointer opaque)
{
    GUnixInputStream *localStdin = G_UNIX_INPUT_STREAM(stream);
    GVirSandboxConsole *console = GVIR_SANDBOX_CONSOLE(opaque);
    GVirSandboxConsolePrivate *priv = console->priv;
    GError *err = NULL;

    gssize ret = g_pollable_input_stream_read_nonblocking
        (G_POLLABLE_INPUT_STREAM(localStdin),
         priv->localToConsole + priv->localToConsoleOffset,
         priv->localToConsoleLength - priv->localToConsoleOffset,
         NULL, &err);
    if (ret < 0) {
        do_console_close(console, err);
        g_error_free(err);
        goto cleanup;
    }

    if (ret == 0) {
        /* There is no concept of 'EOF' on the serial / paravirt
         * console the guest / container is using for its stdio.
         * Thus we send the magic escape sequence '\9'. The
         * libvirt-sandbox-init-common process we notice this
         * and close the stdin of the sandboxed application so
         * that it sees EOF
         */
        if ((priv->localToConsoleLength - priv->localToConsoleOffset) < 2) {
            priv->localToConsole = g_renew(gchar, priv->localToConsole,
                                           priv->localToConsoleLength + 2);
            priv->localToConsoleLength += 2;
        }
        priv->localEOF = TRUE;
        priv->localToConsole[priv->localToConsoleOffset++] = '\\';
        priv->localToConsole[priv->localToConsoleOffset++] = '9';
    }

    priv->localToConsoleOffset += ret;

    priv->localStdinSource = NULL;
    do_console_update_events(console);
cleanup:
    return FALSE;
}


static gboolean do_console_local_write(GObject *stream,
                                       gpointer opaque)
{
    GUnixOutputStream *localStdoutOrStderr = G_UNIX_OUTPUT_STREAM(stream);
    GVirSandboxConsole *console = GVIR_SANDBOX_CONSOLE(opaque);
    GVirSandboxConsolePrivate *priv = console->priv;
    GError *err = NULL;

    gssize ret = g_pollable_output_stream_write_nonblocking
        (G_POLLABLE_OUTPUT_STREAM(localStdoutOrStderr),
         priv->consoleToLocal,
         priv->consoleToLocalOffset,
         NULL, &err);
    if (ret < 0) {
        do_console_close(console, err);
        g_error_free(err);
        goto cleanup;
    }

    if (priv->consoleToLocalOffset != ret)
        memmove(priv->consoleToLocal,
                priv->consoleToLocal + ret,
                priv->consoleToLocalOffset - ret);
    priv->consoleToLocalOffset -= ret;

    priv->localStdoutSource = NULL;
    do_console_update_events(console);
cleanup:
    return FALSE;
}


gboolean gvir_sandbox_console_attach(GVirSandboxConsole *console,
                                      GUnixInputStream *localStdin,
                                      GUnixOutputStream *localStdout,
                                      GUnixOutputStream *localStderr,
                                      GError **error)
{
    GVirSandboxConsolePrivate *priv = console->priv;
    gboolean ret = FALSE;
    virDomainPtr dom;
    virStreamPtr st;

    if (priv->localStdin) {
        g_set_error(error, GVIR_SANDBOX_CONSOLE_ERROR, 0, "%s",
                    "Console is already attached to a stream");
        return FALSE;
    }

    if (!gvir_sandbox_console_start_term(console, localStdin, error))
        return FALSE;

    priv->localStdin = g_object_ref(localStdin);
    priv->localStdout = g_object_ref(localStdout);
    priv->localStderr = g_object_ref(localStderr);

    priv->console = gvir_connection_get_stream(priv->connection, 0);

    g_object_get(priv->domain, "handle", &dom, NULL);
    g_object_get(priv->console, "handle", &st, NULL);

    if (virDomainOpenConsole(dom, "serial0", st, 0) < 0) {
        gvir_error_new(GVIR_SANDBOX_CONSOLE_ERROR, 0,
                       "%s", "Cannot open console");
        goto cleanup;
    }

    priv->consoleToLocalLength = priv->localToConsoleLength = 4096;
    priv->consoleToLocal = g_new0(gchar, priv->consoleToLocalLength);
    priv->localToConsole = g_new0(gchar, priv->localToConsoleLength);

    do_console_update_events(console);

    ret = TRUE;
cleanup:
    if (!ret)
        gvir_sandbox_console_stop_term(console, localStdin, NULL);
    return ret;
}


gboolean gvir_sandbox_console_attach_stdio(GVirSandboxConsole *console,
                                            GError **error)
{
    GInputStream *localStdin = g_unix_input_stream_new(STDIN_FILENO, FALSE);
    GOutputStream *localStdout = g_unix_output_stream_new(STDOUT_FILENO, FALSE);
    GOutputStream *localStderr = g_unix_output_stream_new(STDERR_FILENO, FALSE);
    gboolean ret;

    ret = gvir_sandbox_console_attach(console,
                                       G_UNIX_INPUT_STREAM(localStdin),
                                       G_UNIX_OUTPUT_STREAM(localStdout),
                                       G_UNIX_OUTPUT_STREAM(localStderr),
                                       error);

    g_object_unref(localStdin);
    g_object_unref(localStdout);
    g_object_unref(localStderr);

    return ret;
}


gboolean gvir_sandbox_console_detach(GVirSandboxConsole *console,
                                         GError **error)
{
    GVirSandboxConsolePrivate *priv = console->priv;
    gboolean ret = FALSE;
    if (!priv->localStdin) {
        return TRUE;
#if 0
        g_set_error(error, GVIR_SANDBOX_CONSOLE_ERROR, 0, "%s",
                    "Console is not attached to a stream");
        return FALSE;
#endif
    }

    if (!gvir_sandbox_console_stop_term(console, priv->localStdin, error))
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

    g_object_unref(priv->localStdin);
    g_object_unref(priv->localStdout);
    g_object_unref(priv->localStderr);
    priv->localStdin = NULL;
    priv->localStdout = NULL;
    priv->localStderr = NULL;

    ret = TRUE;

//cleanup:
    return ret;
}
