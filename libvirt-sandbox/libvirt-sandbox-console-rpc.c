/*
 * libvirt-sandbox-console-rpc.c: libvirt sandbox rpc console
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

#include "libvirt-sandbox/libvirt-sandbox-rpcpacket.h"

/**
 * SECTION: libvirt-sandbox-console-rpc
 * @short_description: A text mode rpc console
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

#define GVIR_SANDBOX_CONSOLE_RPC_GET_PRIVATE(obj)                       \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), GVIR_SANDBOX_TYPE_CONSOLE_RPC, GVirSandboxConsoleRpcPrivate))

static gboolean gvir_sandbox_console_rpc_attach(GVirSandboxConsole *console,
                                                GUnixInputStream *localStdin,
                                                GUnixOutputStream *localStdout,
                                                GUnixOutputStream *localStderr,
                                                GError **error);
static gboolean gvir_sandbox_console_rpc_detach(GVirSandboxConsole *console,
                                                GError **error);

typedef enum {
    GVIR_SANDBOX_CONSOLE_RPC_INIT,
    GVIR_SANDBOX_CONSOLE_RPC_WAITING,
    GVIR_SANDBOX_CONSOLE_RPC_SYNCING,
    GVIR_SANDBOX_CONSOLE_RPC_RUNNING,
    GVIR_SANDBOX_CONSOLE_RPC_FINISHED,
} GVirSandboxConsoleRpcState;

struct _GVirSandboxConsoleRpcPrivate
{
    GVirStream *console;

    GUnixInputStream *localStdin;
    GUnixOutputStream *localStdout;
    GUnixOutputStream *localStderr;

    gboolean termiosActive;
    struct termios termiosProps;

    /* Encoded RPC messages, being sent/received */
    GVirSandboxRPCPacket *rx;
    GVirSandboxRPCPacket *tx;

    /* Decoded RPC message forwarded to stdout */
    gchar *localToStdout;
    gsize localToStdoutLength;
    gsize localToStdoutOffset;

    /* Decoded RPC message forwarded to stdout */
    gchar *localToStderr;
    gsize localToStderrLength;
    gsize localToStderrOffset;

    GVirSandboxConsoleRpcState state;

    /* True if stdin has shown us EOF */
    gboolean localEOF;
    /* True if we got the EXIT RPC message over console */
    gboolean consoleEOF;

    GSource *localStdinSource;
    GSource *localStdoutSource;
    GSource *localStderrSource;
    gint consoleWatch;

    gsize serial;
};

G_DEFINE_TYPE(GVirSandboxConsoleRpc, gvir_sandbox_console_rpc, GVIR_SANDBOX_TYPE_CONSOLE);


enum {
    LAST_SIGNAL
};

//static gint signals[LAST_SIGNAL];

#define GVIR_SANDBOX_CONSOLE_RPC_ERROR gvir_sandbox_console_rpc_error_quark()

static GQuark
gvir_sandbox_console_rpc_error_quark(void)
{
    return g_quark_from_static_string("gvir-sandbox-console-rpc");
}

static void gvir_sandbox_console_rpc_send_handshake_wait(GVirSandboxConsoleRpc *console)
{
    GVirSandboxConsoleRpcPrivate *priv = console->priv;
    GVirSandboxRPCPacket *pkt = gvir_sandbox_rpcpacket_new(FALSE);

    g_debug("Send wait");
    pkt->buffer[0] = GVIR_SANDBOX_PROTOCOL_HANDSHAKE_WAIT;
    pkt->bufferLength = 1;
    pkt->bufferOffset = 0;

    priv->tx = pkt;
}


static void gvir_sandbox_console_rpc_send_handshake_sync(GVirSandboxConsoleRpc *console)
{
    GVirSandboxConsoleRpcPrivate *priv = console->priv;
    GVirSandboxRPCPacket *pkt = gvir_sandbox_rpcpacket_new(FALSE);

    g_debug("Send sync");
    pkt->buffer[0] = GVIR_SANDBOX_PROTOCOL_HANDSHAKE_SYNC;
    pkt->bufferLength = 1;
    pkt->bufferOffset = 0;

    priv->tx = pkt;
}


static gboolean gvir_sandbox_console_rpc_send_quit(GVirSandboxConsoleRpc *console,
                                                   GError **error)
{
    GVirSandboxConsoleRpcPrivate *priv = console->priv;
    GVirSandboxRPCPacket *pkt = gvir_sandbox_rpcpacket_new(FALSE);
    gboolean ret = FALSE;

    g_debug("Send quit");
    pkt->header.proc = GVIR_SANDBOX_PROTOCOL_PROC_QUIT;
    pkt->header.status = GVIR_SANDBOX_PROTOCOL_STATUS_OK;
    pkt->header.type = GVIR_SANDBOX_PROTOCOL_TYPE_DATA;
    pkt->header.serial = priv->serial++;

    if (!gvir_sandbox_rpcpacket_encode_header(pkt, error))
        goto cleanup;
    if (!gvir_sandbox_rpcpacket_encode_payload_empty(pkt, error))
        goto cleanup;

    priv->tx = pkt;
    ret = TRUE;

cleanup:
    return ret;
}



static gboolean gvir_sandbox_console_rpc_send_stdin(GVirSandboxConsoleRpc *console,
                                                    gchar *data,
                                                    gsize len,
                                                    GError **error)
{
    GVirSandboxConsoleRpcPrivate *priv = console->priv;
    GVirSandboxRPCPacket *pkt = gvir_sandbox_rpcpacket_new(FALSE);
    gboolean ret = FALSE;

    g_debug("Send stdin %p %zu", data, len);
    pkt->header.proc = GVIR_SANDBOX_PROTOCOL_PROC_STDIN;
    pkt->header.status = GVIR_SANDBOX_PROTOCOL_STATUS_OK;
    pkt->header.type = GVIR_SANDBOX_PROTOCOL_TYPE_DATA;
    pkt->header.serial = priv->serial++;

    if (!gvir_sandbox_rpcpacket_encode_header(pkt, error))
        goto cleanup;
    if (!gvir_sandbox_rpcpacket_encode_payload_raw(pkt, data, len, error))
        goto cleanup;

    priv->tx = pkt;
    ret = TRUE;

cleanup:
    if (!ret)
        gvir_sandbox_rpcpacket_free(pkt);
    return ret;
}


static void gvir_sandbox_console_rpc_finalize(GObject *object)
{
    GVirSandboxConsoleRpc *console = GVIR_SANDBOX_CONSOLE_RPC(object);
    GVirSandboxConsoleRpcPrivate *priv = console->priv;

    if (priv->localStdin)
        gvir_sandbox_console_detach(GVIR_SANDBOX_CONSOLE(console), NULL);

    /* All other private fields are free'd by the detach call */

    gvir_sandbox_rpcpacket_free(priv->tx);
    gvir_sandbox_rpcpacket_free(priv->rx);

    g_free(priv->localToStdout);
    g_free(priv->localToStderr);

    G_OBJECT_CLASS(gvir_sandbox_console_rpc_parent_class)->finalize(object);
}


static void gvir_sandbox_console_rpc_class_init(GVirSandboxConsoleRpcClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    GVirSandboxConsoleClass *console_class = GVIR_SANDBOX_CONSOLE_CLASS(klass);

    object_class->finalize = gvir_sandbox_console_rpc_finalize;
    console_class->attach = gvir_sandbox_console_rpc_attach;
    console_class->detach = gvir_sandbox_console_rpc_detach;

    g_signal_new("exited",
                 G_OBJECT_CLASS_TYPE(object_class),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(GVirSandboxConsoleRpcClass, exited),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__INT,
                 G_TYPE_NONE,
                 1,
                 G_TYPE_INT);

    g_type_class_add_private(klass, sizeof(GVirSandboxConsoleRpcPrivate));
}


static void gvir_sandbox_console_rpc_init(GVirSandboxConsoleRpc *console)
{
    console->priv = GVIR_SANDBOX_CONSOLE_RPC_GET_PRIVATE(console);
}


/**
 * gvir_sandbox_console_rpc_new:
 * @connection: (transfer none): the libvirt connection
 * @domain: (transfer none): the libvirt domain whose console_rpc to run
 * @devname: the console to connect to
 *
 * Create a new sandbox rpc console from the specified configuration
 *
 * Returns: (transfer full): a new sandbox console object
 */
GVirSandboxConsoleRpc *gvir_sandbox_console_rpc_new(GVirConnection *connection,
                                                    GVirDomain *domain,
                                                    const char *devname)
{
    return GVIR_SANDBOX_CONSOLE_RPC(g_object_new(GVIR_SANDBOX_TYPE_CONSOLE_RPC,
                                                 "connection", connection,
                                                 "domain", domain,
                                                 "devname", devname,
                                                 NULL));
}


static gboolean gvir_sandbox_console_rpc_start_term(GVirSandboxConsoleRpc *console,
                                                    GUnixInputStream *localStdin,
                                                    GError **error)
{
    GVirSandboxConsoleRpcPrivate *priv = console->priv;
    int fd;
    struct termios ios;

    if (!localStdin)
        return TRUE;

    fd = g_unix_input_stream_get_fd(localStdin);

    if (!isatty(fd))
        return TRUE;

    if (tcgetattr(fd, &priv->termiosProps) < 0) {
        g_set_error(error, GVIR_SANDBOX_CONSOLE_RPC_ERROR, 0,
                    "Unable to query terminal attributes: %s",
                    strerror(errno));
        return FALSE;
    }

    memcpy(&ios, &priv->termiosProps, sizeof(ios));

    /* Put into rpc mode */
    ios.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP |
                     INLCR | IGNCR | ICRNL | IXON);
    ios.c_oflag &= ~(OPOST);
    ios.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    ios.c_cflag &= ~(CSIZE |PARENB);
    ios.c_cflag |= CS8;

    if (tcsetattr(fd, TCSADRAIN, &ios) < 0) {
        g_set_error(error, GVIR_SANDBOX_CONSOLE_RPC_ERROR, 0,
                    "Unable to update terminal attributes: %s",
                    strerror(errno));
        return FALSE;
    }

    priv->termiosActive = TRUE;

    return TRUE;
}


static gboolean gvir_sandbox_console_rpc_stop_term(GVirSandboxConsoleRpc *console,
                                                   GUnixInputStream *localStdin,
                                                   GError **error)
{
    GVirSandboxConsoleRpcPrivate *priv = console->priv;
    int fd;

    if (!localStdin)
        return TRUE;

    fd = g_unix_input_stream_get_fd(localStdin);

    if (!isatty(fd))
        return TRUE;

    if (tcsetattr(fd, TCSADRAIN, &priv->termiosProps) < 0) {
        g_set_error(error, GVIR_SANDBOX_CONSOLE_RPC_ERROR, 0,
                    "Unable to restore terminal attributes: %s",
                    strerror(errno));
        return FALSE;
    }

    priv->termiosActive = FALSE;

    return TRUE;
}


static gboolean do_console_rpc_stream_readwrite(GVirStream *stream,
                                                GVirStreamIOCondition cond,
                                                gpointer opaque);
static gboolean do_console_rpc_stdin_read(GObject *stream,
                                          gpointer opaque);
static gboolean do_console_rpc_stdout_write(GObject *stream,
                                            gpointer opaque);
static gboolean do_console_rpc_stderr_write(GObject *stream,
                                            gpointer opaque);

static void do_console_rpc_update_events(GVirSandboxConsoleRpc *console)
{
    GVirSandboxConsoleRpcPrivate *priv = console->priv;
    GVirStreamIOCondition cond = 0;

    if (priv->state == GVIR_SANDBOX_CONSOLE_RPC_INIT)
        return;

    if (priv->state == GVIR_SANDBOX_CONSOLE_RPC_RUNNING) {
        /* If nothing is waiting to be sent to guest, we can read
         * some more of stdin */
        if (!priv->tx && !priv->localEOF) {
            if (priv->localStdinSource == NULL && priv->localStdin) {
                priv->localStdinSource = g_pollable_input_stream_create_source
                    (G_POLLABLE_INPUT_STREAM(priv->localStdin), NULL);
                g_source_set_callback(priv->localStdinSource,
                                      (GSourceFunc)do_console_rpc_stdin_read,
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

        /* If we have data queued for stdout, we better pull to write */
        if (priv->localToStdoutLength) {
            if (priv->localStdoutSource == NULL) {
                priv->localStdoutSource = g_pollable_output_stream_create_source
                    (G_POLLABLE_OUTPUT_STREAM(priv->localStdout), NULL);
                g_source_set_callback(priv->localStdoutSource,
                                      (GSourceFunc)do_console_rpc_stdout_write,
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

        /* If we have data queued for stderr, we better pull to write */
        if (priv->localToStderrLength) {
            if (priv->localStderrSource == NULL) {
                priv->localStderrSource = g_pollable_output_stream_create_source
                    (G_POLLABLE_OUTPUT_STREAM(priv->localStderr), NULL);
                g_source_set_callback(priv->localStderrSource,
                                      (GSourceFunc)do_console_rpc_stderr_write,
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

    /* If we can RPC ready for TX we must write*/
    if (priv->tx)
        cond |= GVIR_STREAM_IO_CONDITION_WRITABLE;
    if (priv->rx)
        cond |= GVIR_STREAM_IO_CONDITION_READABLE;

    if (priv->consoleWatch) {
        g_source_remove(priv->consoleWatch);
        priv->consoleWatch = 0;
    }

    if (cond)
        priv->consoleWatch = gvir_stream_add_watch(priv->console,
                                                   cond,
                                                   do_console_rpc_stream_readwrite,
                                                   console);
}

static void do_console_rpc_close(GVirSandboxConsoleRpc *console,
                                 GError *error)
{
    gvir_sandbox_console_detach(GVIR_SANDBOX_CONSOLE(console), NULL);
    g_signal_emit_by_name(console, "closed", error != NULL);
}

static gboolean do_console_rpc_dispatch(GVirSandboxConsoleRpc *console,
                                        GError **error)
{
    GVirSandboxConsoleRpcPrivate *priv = console->priv;
    struct GVirSandboxProtocolMessageExit msgexit;

    if (!gvir_sandbox_rpcpacket_decode_header(priv->rx, error))
        return FALSE;

    if (priv->rx->header.status != GVIR_SANDBOX_PROTOCOL_STATUS_OK) {
        g_set_error(error, 0, 0, "Unexpected rpc status %u",
                    priv->rx->header.status);
        return FALSE;
    }

    switch (priv->rx->header.proc) {
    case GVIR_SANDBOX_PROTOCOL_PROC_STDOUT:
        priv->localToStdoutOffset = 0;
        priv->localToStdoutLength = priv->rx->bufferLength - priv->rx->bufferOffset;
        priv->localToStdout = g_new0(gchar, priv->localToStdoutLength);

        memcpy(priv->localToStdout,
               priv->rx->buffer + priv->rx->bufferOffset,
               priv->localToStdoutLength);
        break;

    case GVIR_SANDBOX_PROTOCOL_PROC_STDERR:
        priv->localToStderrOffset = 0;
        priv->localToStderrLength = priv->rx->bufferLength - priv->rx->bufferOffset;
        priv->localToStderr = g_new0(gchar, priv->localToStderrLength);

        memcpy(priv->localToStderr,
               priv->rx->buffer + priv->rx->bufferOffset,
               priv->localToStderrLength);
        break;

    case GVIR_SANDBOX_PROTOCOL_PROC_EXIT:
        memset(&msgexit, 0, sizeof(msgexit));
        if (!(gvir_sandbox_rpcpacket_decode_payload_msg(priv->rx,
                                                        (xdrproc_t)xdr_GVirSandboxProtocolMessageExit,
                                                        (void*)&msgexit,
                                                        error)))
            return FALSE;
        g_debug("Switch state to finished");
        priv->state = GVIR_SANDBOX_CONSOLE_RPC_FINISHED;
        priv->consoleEOF = TRUE;

        g_signal_emit_by_name(console, "exited", msgexit.status);
        if (!gvir_sandbox_console_rpc_send_quit(console, error))
            return FALSE;
        break;

    case GVIR_SANDBOX_PROTOCOL_PROC_QUIT:
    case GVIR_SANDBOX_PROTOCOL_PROC_STDIN:
    default:
        g_set_error(error, 0, 0, "Unexpected rpc proc %u",
                    priv->rx->header.proc);
        return FALSE;
    }
    return TRUE;
}


static gboolean do_console_handshake_wait(gpointer opaque)
{
    GVirSandboxConsoleRpc *console = GVIR_SANDBOX_CONSOLE_RPC(opaque);
    GVirSandboxConsoleRpcPrivate *priv = console->priv;

    if (priv->state == GVIR_SANDBOX_CONSOLE_RPC_WAITING)
        gvir_sandbox_console_rpc_send_handshake_wait(GVIR_SANDBOX_CONSOLE_RPC(console));
    else
        gvir_sandbox_console_rpc_send_handshake_sync(GVIR_SANDBOX_CONSOLE_RPC(console));

    do_console_rpc_update_events(console);

    return FALSE;
}


static gboolean do_console_rpc_stream_readwrite(GVirStream *stream,
                                                GVirStreamIOCondition cond,
                                                gpointer opaque)
{
    GVirSandboxConsoleRpc *console = GVIR_SANDBOX_CONSOLE_RPC(opaque);
    GVirSandboxConsoleRpcPrivate *priv = console->priv;

readmore:
    if ((cond & GVIR_STREAM_IO_CONDITION_READABLE) &&
        priv->rx) {
        GError *err = NULL;
        gssize ret = gvir_stream_receive
            (stream,
             priv->rx->buffer + priv->rx->bufferOffset,
             priv->rx->bufferLength - priv->rx->bufferOffset,
             NULL,
             &err);
        if (ret < 0) {
            if (err && err->code == G_IO_ERROR_WOULD_BLOCK) {
                /* Shouldn't get this, but you never know */
                g_error_free(err);
                goto done;
            } else {
                g_debug("Error from stream recv %s", err ? err->message : "");
                do_console_rpc_close(console, err);
                g_error_free(err);
                goto cleanup;
            }
        }
        if (ret == 0) { /* EOF */
            goto done;
        }
        priv->rx->bufferOffset += ret;

        if (priv->rx->bufferOffset == priv->rx->bufferLength) {
            switch (priv->state) {
            case GVIR_SANDBOX_CONSOLE_RPC_WAITING:
                if (priv->rx->buffer[0] == GVIR_SANDBOX_PROTOCOL_HANDSHAKE_SYNC) {
                    g_debug("Switch state to syncing");
                    priv->state = GVIR_SANDBOX_CONSOLE_RPC_SYNCING;
                    gvir_sandbox_rpcpacket_free(priv->rx);
                    priv->rx = NULL;
                } else {
                    /* Try recv another byte */
                    priv->rx->bufferOffset = 0;
                    priv->rx->bufferLength = 1;
                }
                break;

            case GVIR_SANDBOX_CONSOLE_RPC_RUNNING:
            case GVIR_SANDBOX_CONSOLE_RPC_FINISHED:
                if (priv->rx->bufferLength == GVIR_SANDBOX_PROTOCOL_LEN_MAX) {
                    if (!gvir_sandbox_rpcpacket_decode_length(priv->rx, &err)) {
                        g_debug("Error from decode length %s", err ? err->message : "");
                        do_console_rpc_close(console, err);
                        g_error_free(err);
                        goto cleanup;
                    }
                    goto readmore;
                } else {
                    if (!do_console_rpc_dispatch(console, &err)) {
                        g_debug("Error from dispatch %s", err ? err->message : "");
                        do_console_rpc_close(console, err);
                        g_error_free(err);
                        goto cleanup;
                    }
                    gvir_sandbox_rpcpacket_free(priv->rx);
                    priv->rx = NULL;
                }
                break;

            case GVIR_SANDBOX_CONSOLE_RPC_INIT:
            case GVIR_SANDBOX_CONSOLE_RPC_SYNCING:
            default:
                g_debug("Got rx in unexpected state %d", priv->state);
                do_console_rpc_close(console, err);
                g_error_free(err);
                goto cleanup;
            }
        }
    }

    if ((cond & GVIR_STREAM_IO_CONDITION_WRITABLE) &&
        priv->tx) {
        GError *err = NULL;
        gssize ret = gvir_stream_send(stream,
                                      priv->tx->buffer + priv->tx->bufferOffset,
                                      priv->tx->bufferLength - priv->tx->bufferOffset,
                                      NULL,
                                      &err);
        if (ret < 0) {
            g_debug("Error from stream send %s", err ? err->message : "");
            do_console_rpc_close(console, err);
            g_error_free(err);
            goto cleanup;
        }

        priv->tx->bufferOffset += ret;
        if (priv->tx->bufferOffset == priv->tx->bufferLength) {
            if (priv->state == GVIR_SANDBOX_CONSOLE_RPC_WAITING) {
                g_debug("Schedule another wait");
                g_timeout_add(50,
                              do_console_handshake_wait,
                              console);
            } else if (priv->state == GVIR_SANDBOX_CONSOLE_RPC_SYNCING) {
                if (priv->tx->buffer[0] == GVIR_SANDBOX_PROTOCOL_HANDSHAKE_WAIT) {
                    gvir_sandbox_console_rpc_send_handshake_sync(GVIR_SANDBOX_CONSOLE_RPC(console));
                } else {
                    g_debug("Switch state to running");
                    priv->state = GVIR_SANDBOX_CONSOLE_RPC_RUNNING;
                    priv->rx = gvir_sandbox_rpcpacket_new(TRUE);
                }
            } else if (priv->state == GVIR_SANDBOX_CONSOLE_RPC_FINISHED) {
                if (!priv->localToStdout &&
                    !priv->localToStderr) {
                    g_debug("Transmitted all data, with last pkt, done");
                    do_console_rpc_close(console, NULL);
                }
            }
            gvir_sandbox_rpcpacket_free(priv->tx);
            priv->tx = NULL;
        }
    }

done:
    priv->consoleWatch = 0;
    do_console_rpc_update_events(console);

cleanup:
    return FALSE;
}

/*
 * Convert given character to control character.
 * Basically, we assume ASCII, and take lower 6 bits.
 */
#define CONTROL(c) ((c) ^ 0x40)

#define MAX_IO 4096

static gboolean do_console_rpc_stdin_read(GObject *stream,
                                          gpointer opaque)
{
    GUnixInputStream *localStdin = G_UNIX_INPUT_STREAM(stream);
    GVirSandboxConsoleRpc *console = GVIR_SANDBOX_CONSOLE_RPC(opaque);
    GVirSandboxConsoleRpcPrivate *priv = console->priv;
    GError *err = NULL;
    gchar *buf = g_new0(gchar, MAX_IO);

    gssize ret = g_pollable_input_stream_read_nonblocking
        (G_POLLABLE_INPUT_STREAM(localStdin),
         buf, MAX_IO,
         NULL, &err);
    if (ret < 0) {
        g_debug("Error from local read %s", err ? err->message : "");
        do_console_rpc_close(console, err);
        g_error_free(err);
        goto cleanup;
    }

    if (ret == 0)
        priv->localEOF = TRUE;
    else if (buf[0] ==
             CONTROL(gvir_sandbox_console_get_escape(GVIR_SANDBOX_CONSOLE(console)))) {
        do_console_rpc_close(console, err);
        goto cleanup;
    }

    if (!gvir_sandbox_console_rpc_send_stdin(console, buf, ret, &err)) {
        g_debug("Error from RPC encode send %s", err ? err->message : "");
        do_console_rpc_close(console, err);
        g_error_free(err);
        goto cleanup;
    }
    priv->localStdinSource = NULL;
cleanup:
    do_console_rpc_update_events(console);
    g_free(buf);
    return FALSE;
}


static gboolean do_console_rpc_stdout_write(GObject *stream,
                                            gpointer opaque)
{
    GUnixOutputStream *localStdout = G_UNIX_OUTPUT_STREAM(stream);
    GVirSandboxConsoleRpc *console = GVIR_SANDBOX_CONSOLE_RPC(opaque);
    GVirSandboxConsoleRpcPrivate *priv = console->priv;
    GError *err = NULL;

    gssize ret = g_pollable_output_stream_write_nonblocking
        (G_POLLABLE_OUTPUT_STREAM(localStdout),
         priv->localToStdout + priv->localToStdoutOffset,
         priv->localToStdoutLength - priv->localToStdoutOffset,
         NULL, &err);
    if (ret < 0) {
        g_debug("Error from local write %s", err ? err->message : "");
        do_console_rpc_close(console, err);
        g_error_free(err);
        goto cleanup;
    }

    priv->localToStdoutOffset += ret;

    if (priv->localToStdoutOffset == priv->localToStdoutLength) {
        g_free(priv->localToStdout);
        priv->localToStdout = NULL;
        priv->localToStdoutOffset = priv->localToStdoutLength = 0;

        if (!priv->consoleEOF)
            priv->rx = gvir_sandbox_rpcpacket_new(TRUE);

        if (priv->state == GVIR_SANDBOX_CONSOLE_RPC_FINISHED &&
            priv->tx == NULL &&
            priv->localToStderr == NULL) {
            g_debug("Transmitted all data, with last stdout, done");
            do_console_rpc_close(console, err);
        }
    }
    priv->localStdoutSource = NULL;
    do_console_rpc_update_events(console);

cleanup:
    return FALSE;
}


static gboolean do_console_rpc_stderr_write(GObject *stream,
                                            gpointer opaque)
{
    GUnixOutputStream *localStderr = G_UNIX_OUTPUT_STREAM(stream);
    GVirSandboxConsoleRpc *console = GVIR_SANDBOX_CONSOLE_RPC(opaque);
    GVirSandboxConsoleRpcPrivate *priv = console->priv;
    GError *err = NULL;
    gssize ret = g_pollable_output_stream_write_nonblocking
        (G_POLLABLE_OUTPUT_STREAM(localStderr),
         priv->localToStderr,
         priv->localToStderrOffset,
         NULL, &err);
    if (ret < 0) {
        g_debug("Error from local write %s", err ? err->message : "");
        do_console_rpc_close(console, err);
        g_error_free(err);
        goto cleanup;
    }

    priv->localToStderrOffset += ret;

    if (priv->localToStderrOffset == priv->localToStderrLength) {
        g_free(priv->localToStderr);
        priv->localToStderr = NULL;
        priv->localToStderrOffset = priv->localToStderrLength = 0;

        if (!priv->consoleEOF)
            priv->rx = gvir_sandbox_rpcpacket_new(TRUE);

        if (priv->state == GVIR_SANDBOX_CONSOLE_RPC_FINISHED &&
            priv->tx == NULL &&
            priv->localToStdout == NULL) {
            g_debug("Transmitted all data, with last stderr, done");
            do_console_rpc_close(console, err);
        }
    }

    priv->localStderrSource = NULL;
    do_console_rpc_update_events(console);

cleanup:
    return FALSE;
}


static gboolean gvir_sandbox_console_rpc_attach(GVirSandboxConsole *console,
                                                GUnixInputStream *localStdin,
                                                GUnixOutputStream *localStdout,
                                                GUnixOutputStream *localStderr,
                                                GError **error)
{
    GVirSandboxConsoleRpcPrivate *priv = GVIR_SANDBOX_CONSOLE_RPC(console)->priv;
    gboolean ret = FALSE;
    GVirConnection *conn = NULL;
    GVirDomain *dom = NULL;
    gchar *devname = NULL;

    if (priv->state != GVIR_SANDBOX_CONSOLE_RPC_INIT) {
        g_set_error(error, GVIR_SANDBOX_CONSOLE_RPC_ERROR, 0, "%s",
                    "Console is already attached to a stream");
        return FALSE;
    }

    if (!gvir_sandbox_console_rpc_start_term(GVIR_SANDBOX_CONSOLE_RPC(console),
                                             localStdin, error))
        return FALSE;

    if (localStdin)
        priv->localStdin = g_object_ref(localStdin);

    priv->localStdout = g_object_ref(localStdout);
    priv->localStderr = g_object_ref(localStderr);

    g_object_get(console,
                 "connection", &conn,
                 "domain", &dom,
                 "devname", &devname,
                 NULL);

    priv->console = gvir_connection_get_stream(conn, 0);

    if (!gvir_domain_open_console(dom, priv->console,
                                  devname, 0, error))
        goto cleanup;

    gvir_sandbox_console_rpc_send_handshake_wait(GVIR_SANDBOX_CONSOLE_RPC(console));
    g_debug("Switch state to waiting");
    priv->state = GVIR_SANDBOX_CONSOLE_RPC_WAITING;
    priv->rx = gvir_sandbox_rpcpacket_new(FALSE);
    priv->rx->bufferLength = 1; /* We need to recv a hanshake byte first */

    do_console_rpc_update_events(GVIR_SANDBOX_CONSOLE_RPC(console));

    ret = TRUE;
cleanup:
    if (!ret)
        gvir_sandbox_console_rpc_stop_term(GVIR_SANDBOX_CONSOLE_RPC(console),
                                           localStdin, NULL);

    if (conn)
        g_object_unref(conn);
    if (dom)
        g_object_unref(dom);
    g_free(devname);
    return ret;
}


static gboolean gvir_sandbox_console_rpc_detach(GVirSandboxConsole *console,
                                                GError **error)
{
    GVirSandboxConsoleRpcPrivate *priv = GVIR_SANDBOX_CONSOLE_RPC(console)->priv;
    gboolean ret = FALSE;

    if (priv->state == GVIR_SANDBOX_CONSOLE_RPC_INIT) {
        return TRUE;
#if 0
        g_set_error(error, GVIR_SANDBOX_CONSOLE_RPC_ERROR, 0, "%s",
                    "ConsoleRpc is not attached to a stream");
        return FALSE;
#endif
    }

    if (!gvir_sandbox_console_rpc_stop_term(GVIR_SANDBOX_CONSOLE_RPC(console),
                                            priv->localStdin, error))
        return FALSE;

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
    g_object_unref(priv->localStdout);
    g_object_unref(priv->localStderr);
    priv->localStdin = NULL;
    priv->localStdout = NULL;
    priv->localStderr = NULL;

    g_free(priv->localToStdout);
    g_free(priv->localToStderr);
    priv->localToStdout = priv->localToStderr = NULL;
    priv->localToStdoutLength = priv->localToStdoutOffset = 0;
    priv->localToStderrLength = priv->localToStderrOffset = 0;

    gvir_sandbox_rpcpacket_free(priv->tx);
    gvir_sandbox_rpcpacket_free(priv->rx);
    priv->tx = priv->rx = NULL;

    priv->state = GVIR_SANDBOX_CONSOLE_RPC_INIT;

    ret = TRUE;

//cleanup:
    return ret;
}
