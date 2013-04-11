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

#include <glib/gi18n.h>
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
    /*
     * No streams connected
     */
    GVIR_SANDBOX_CONSOLE_RPC_STATE_INACTIVE = 0,

    /*
     * Remote stream connected, need tx/rx.
     *
     *  - Sending GVIR_SANDBOX_PROTOCOL_HANDSHAKE_WAIT bytes every 50ms
     *
     * If receive GVIR_SANDBOX_PROTOCOL_HANDSHAKE_WAIT byte, switch
     * to next state
     */
    GVIR_SANDBOX_CONSOLE_RPC_STATE_WAITING = 1,

    /*
     * Remote stream connected, need tx
     *
     *  - Send single GVIR_SANDBOX_PROTOCOL_HANDSHAKE_SYNC byte
     *
     * When byte is sent, switch to next state
     */
    GVIR_SANDBOX_CONSOLE_RPC_STATE_SYNCING = 2,

    /*
     * Remote stream connected, need tx/rx
     * Local stdin/out/err connected, need tx/rx
     *
     * When receiving PROC_EXIT switch to STOPPING
     */
    GVIR_SANDBOX_CONSOLE_RPC_STATE_RUNNING = 3,

    /*
     * Remote stream connected, need tx
     * Local stdout/err connected, need tx
     *
     * When stream tx == 0 and stdout/err == 0, send
     * PROC_QUIT and move to next state
     */
    GVIR_SANDBOX_CONSOLE_RPC_STATE_STOPPING = 4,

    /*
     * Remote stream connected, need tx
     *
     * When tx == 0, then emit "closed", go back
     * to first state
     */
    GVIR_SANDBOX_CONSOLE_RPC_STATE_FINISHED = 5,
} GVirSandboxConsoleRpcState;


#define GVIR_SANDBOX_CONSOLE_MAX_QUEUED_DATA 1024

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
    gsize localToStdoutLength; /* No more than GVIR_SANDBOX_CONSOLE_MAX_QUEUED_DATA*/
    gsize localToStdoutOffset;

    /* Decoded RPC message forwarded to stdout */
    gchar *localToStderr;
    gsize localToStderrLength; /* No more than GVIR_SANDBOX_CONSOLE_MAX_QUEUED_DATA*/
    gsize localToStderrOffset;

    GVirSandboxConsoleRpcState state;

    /* True if stdin has shown us EOF */
    gboolean localEOF;

    GSource *localStdinSource;
    GSource *localStdoutSource;
    GSource *localStderrSource;
    gint consoleWatch;

    /* True if on a TTY & escape sequence is allowed */
    gboolean allowEscape;

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

static GVirSandboxRPCPacket *
gvir_sandbox_console_rpc_build_handshake_wait(void)
{
    GVirSandboxRPCPacket *pkt = gvir_sandbox_rpcpacket_new(FALSE);

    g_debug("Build wait");
    pkt->buffer[0] = GVIR_SANDBOX_PROTOCOL_HANDSHAKE_WAIT;
    pkt->bufferLength = 1;
    pkt->bufferOffset = 0;

    return pkt;
}


static GVirSandboxRPCPacket *
gvir_sandbox_console_rpc_build_handshake_sync(void)
{
    GVirSandboxRPCPacket *pkt = gvir_sandbox_rpcpacket_new(FALSE);

    g_debug("Build sync");
    pkt->buffer[0] = GVIR_SANDBOX_PROTOCOL_HANDSHAKE_SYNC;
    pkt->bufferLength = 1;
    pkt->bufferOffset = 0;

    return pkt;
}


static GVirSandboxRPCPacket *
gvir_sandbox_console_rpc_build_quit(GVirSandboxConsoleRpc *console,
                                    GError **error)
{
    GVirSandboxConsoleRpcPrivate *priv = console->priv;
    GVirSandboxRPCPacket *pkt = gvir_sandbox_rpcpacket_new(FALSE);

    g_debug("Build quit");
    pkt->header.proc = GVIR_SANDBOX_PROTOCOL_PROC_QUIT;
    pkt->header.status = GVIR_SANDBOX_PROTOCOL_STATUS_OK;
    pkt->header.type = GVIR_SANDBOX_PROTOCOL_TYPE_DATA;
    pkt->header.serial = priv->serial++;

    if (!gvir_sandbox_rpcpacket_encode_header(pkt, error))
        goto error;
    if (!gvir_sandbox_rpcpacket_encode_payload_empty(pkt, error))
        goto error;

    return pkt;

error:
    gvir_sandbox_rpcpacket_free(pkt);
    return NULL;
}



static GVirSandboxRPCPacket *
gvir_sandbox_console_rpc_build_stdin(GVirSandboxConsoleRpc *console,
                                     gchar *data,
                                     gsize len,
                                     GError **error)
{
    GVirSandboxConsoleRpcPrivate *priv = console->priv;
    GVirSandboxRPCPacket *pkt = gvir_sandbox_rpcpacket_new(FALSE);

    g_debug("Build stdin %p %zu", data, len);
    pkt->header.proc = GVIR_SANDBOX_PROTOCOL_PROC_STDIN;
    pkt->header.status = GVIR_SANDBOX_PROTOCOL_STATUS_OK;
    pkt->header.type = GVIR_SANDBOX_PROTOCOL_TYPE_DATA;
    pkt->header.serial = priv->serial++;

    if (!gvir_sandbox_rpcpacket_encode_header(pkt, error))
        goto error;
    if (!gvir_sandbox_rpcpacket_encode_payload_raw(pkt, data, len, error))
        goto error;

    return pkt;

error:
    gvir_sandbox_rpcpacket_free(pkt);
    return NULL;
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

    if (!isatty(fd)) {
        priv->allowEscape = FALSE;
        g_debug("input is not on a tty");
        return TRUE;
    }

    priv->allowEscape = TRUE;
    g_debug("Putting TTY in raw mode");
    if (tcgetattr(fd, &priv->termiosProps) < 0) {
        g_set_error(error, GVIR_SANDBOX_CONSOLE_RPC_ERROR, 0,
                    _("Unable to query terminal attributes: %s"),
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
                    _("Unable to update terminal attributes: %s"),
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
                    _("Unable to restore terminal attributes: %s"),
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

static gboolean do_console_rpc_set_state(GVirSandboxConsoleRpc *console,
                                         GVirSandboxConsoleRpcState state,
                                         GError **err)
{
    GVirSandboxConsoleRpcPrivate *priv = console->priv;

    if (priv->state == state) {
        g_debug("Already in state %d", state);
        return TRUE;
    }
    g_debug("Switch state from %d to %d", priv->state, state);
    priv->state = state;

    switch (priv->state) {
    case GVIR_SANDBOX_CONSOLE_RPC_STATE_WAITING:
        priv->tx = gvir_sandbox_console_rpc_build_handshake_wait();
        priv->rx = gvir_sandbox_rpcpacket_new(FALSE);
        priv->rx->bufferLength = 1; /* We need to recv a hanshake byte */
        break;

    case GVIR_SANDBOX_CONSOLE_RPC_STATE_SYNCING:
        if (priv->tx)
            gvir_sandbox_rpcpacket_free(priv->tx);
        priv->tx = gvir_sandbox_console_rpc_build_handshake_sync();
        break;

    case GVIR_SANDBOX_CONSOLE_RPC_STATE_RUNNING:
        priv->rx = gvir_sandbox_rpcpacket_new(TRUE);
        break;

    case GVIR_SANDBOX_CONSOLE_RPC_STATE_STOPPING:
        /* Container has exited, so no point trying to send any
         * stdin data that might be queued */
        if (priv->tx) {
            gvir_sandbox_rpcpacket_free(priv->tx);
            priv->tx = NULL;
        }
        break;

    case GVIR_SANDBOX_CONSOLE_RPC_STATE_FINISHED:
        if (!(priv->tx = gvir_sandbox_console_rpc_build_quit(console, err)))
            return FALSE;
        break;

    case GVIR_SANDBOX_CONSOLE_RPC_STATE_INACTIVE:
    default:
        break;
    }
    return TRUE;
}

static void do_console_rpc_update_events(GVirSandboxConsoleRpc *console)
{
    GVirSandboxConsoleRpcPrivate *priv = console->priv;
    GVirStreamIOCondition cond = 0;
    gboolean needLocalStdin = FALSE;
    gboolean needLocalStdout = FALSE;
    gboolean needLocalStderr = FALSE;

    g_debug("Update rx=%p tx=%p localeof=%d "
            "stdinsource=%p stdoutsource=%p stderrsource=%p "
            "stdoutlen=%zu stderrlen=%zu",
            priv->rx, priv->tx, priv->localEOF,
            priv->localStdinSource,
            priv->localStdoutSource,
            priv->localStderrSource,
            priv->localToStdoutLength,
            priv->localToStderrLength);

    switch (priv->state) {
    case GVIR_SANDBOX_CONSOLE_RPC_STATE_RUNNING:
        /* If nothing is waiting to be sent to guest, we can read
         * some more of stdin */
        if (!priv->tx && !priv->localEOF)
            needLocalStdin = TRUE;

        /* Fall through */

    case GVIR_SANDBOX_CONSOLE_RPC_STATE_STOPPING:
        /* If we have data queued for stdout, we better poll to write */
        if (priv->localToStdoutLength)
            needLocalStdout = TRUE;

        /* If we have data queued for stderr, we better poll to write */
        if (priv->localToStderrLength)
            needLocalStderr = TRUE;

        /* Fall through */

    case GVIR_SANDBOX_CONSOLE_RPC_STATE_WAITING:
    case GVIR_SANDBOX_CONSOLE_RPC_STATE_SYNCING:
        /* If we have RPC ready for RX we must read */
        if (priv->rx)
            cond |= GVIR_STREAM_IO_CONDITION_READABLE;

        /* Fall through */

    case GVIR_SANDBOX_CONSOLE_RPC_STATE_FINISHED:
        /* If we have RPC ready for TX we must write */
        if (priv->tx)
            cond |= GVIR_STREAM_IO_CONDITION_WRITABLE;
        break;

    case GVIR_SANDBOX_CONSOLE_RPC_STATE_INACTIVE:
    default:
        break;

    }

    g_debug("new events stdin=%d sdout=%d stderr=%d stream=%d",
            needLocalStdin, needLocalStdout, needLocalStderr, cond);

    if (needLocalStdin) {
        if (priv->localStdinSource == NULL) {
            priv->localStdinSource = g_pollable_input_stream_create_source
                (G_POLLABLE_INPUT_STREAM(priv->localStdin), NULL);
            g_debug("stdint source %p",
                    priv->localStdinSource);
            g_source_set_callback(priv->localStdinSource,
                                  (GSourceFunc)do_console_rpc_stdin_read,
                                  console,
                                  NULL);
            g_source_attach(priv->localStdinSource,
                            g_main_context_default());
        }
    } else {
        if (priv->localStdinSource != NULL) {
            g_debug("Remove stdin %p", priv->localStdinSource);
            g_source_destroy(priv->localStdinSource);
            g_source_unref(priv->localStdinSource);
            priv->localStdinSource = NULL;
        }
    }

    if (needLocalStdout) {
        if (priv->localStdoutSource == NULL) {
            priv->localStdoutSource = g_pollable_output_stream_create_source
                (G_POLLABLE_OUTPUT_STREAM(priv->localStdout), NULL);
            g_debug("stdout source %p",
                    priv->localStdoutSource);
            g_source_set_callback(priv->localStdoutSource,
                                  (GSourceFunc)do_console_rpc_stdout_write,
                                  console,
                                  NULL);
            g_source_attach(priv->localStdoutSource,
                            g_main_context_default());
        }
    } else {
        if (priv->localStdoutSource != NULL) {
            g_debug("Remove stdout %p", priv->localStdoutSource);
            g_source_destroy(priv->localStdoutSource);
            g_source_unref(priv->localStdoutSource);
            priv->localStdoutSource = NULL;
        }
    }

    if (needLocalStderr) {
        if (priv->localStderrSource == NULL) {
            priv->localStderrSource = g_pollable_output_stream_create_source
                (G_POLLABLE_OUTPUT_STREAM(priv->localStderr), NULL);
            g_debug("stderr source %p",
                    priv->localStderrSource);
            g_source_set_callback(priv->localStderrSource,
                                  (GSourceFunc)do_console_rpc_stderr_write,
                                  console,
                                  NULL);
            g_source_attach(priv->localStderrSource,
                            g_main_context_default());
        }
    } else {
        if (priv->localStderrSource != NULL) {
            g_debug("Remove stderr %p", priv->localStderrSource);
            g_source_destroy(priv->localStderrSource);
            g_source_unref(priv->localStderrSource);
            priv->localStderrSource = NULL;
        }
    }

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
                                 GError *err)
{
    g_debug("Closing console %s", err ? err->message : "");
    gvir_sandbox_console_detach(GVIR_SANDBOX_CONSOLE(console), NULL);
    g_signal_emit_by_name(console, "closed", err != NULL);
}

static gboolean do_console_rpc_dispatch_proc(GVirSandboxConsoleRpc *console,
                                             GVirSandboxRPCPacket *pkt,
                                             GError **error)
{
    GVirSandboxConsoleRpcPrivate *priv = console->priv;
    struct GVirSandboxProtocolMessageExit msgexit;
    gsize want;

    if (!gvir_sandbox_rpcpacket_decode_header(pkt, error))
        return FALSE;

    if (pkt->header.status != GVIR_SANDBOX_PROTOCOL_STATUS_OK) {
        g_set_error(error, GVIR_SANDBOX_CONSOLE_RPC_ERROR, 0,
                    _("Unexpected rpc status %u"),
                    pkt->header.status);
        return FALSE;
    }
    //g_debug("Procedure %d", pkt->header.proc);
    switch (pkt->header.proc) {
    case GVIR_SANDBOX_PROTOCOL_PROC_STDOUT:
        want = pkt->bufferLength - pkt->bufferOffset;
        if (!priv->localToStdout) {
            priv->localToStdoutOffset = 0;
            priv->localToStdoutLength = 0;
            priv->localToStdout = g_new0(gchar, want);
        } else {
            priv->localToStdout = g_renew(gchar,
                                          priv->localToStdout,
                                          priv->localToStdoutLength + want);
        }
        memcpy(priv->localToStdout + priv->localToStdoutLength,
               pkt->buffer + pkt->bufferOffset,
               want);
        priv->localToStdoutLength += want;
        break;

    case GVIR_SANDBOX_PROTOCOL_PROC_STDERR:
        want = pkt->bufferLength - pkt->bufferOffset;
        if (!priv->localToStderr) {
            priv->localToStderrOffset = 0;
            priv->localToStderrLength = 0;
            priv->localToStderr = g_new0(gchar, want);
        } else {
            priv->localToStderr = g_renew(gchar,
                                          priv->localToStderr,
                                          priv->localToStderrLength + want);
        }

        memcpy(priv->localToStderr + priv->localToStderrLength,
               pkt->buffer + pkt->bufferOffset,
               want);
        priv->localToStderrLength += want;
        break;

    case GVIR_SANDBOX_PROTOCOL_PROC_EXIT:
        memset(&msgexit, 0, sizeof(msgexit));
        if (!(gvir_sandbox_rpcpacket_decode_payload_msg(pkt,
                                                        (xdrproc_t)xdr_GVirSandboxProtocolMessageExit,
                                                        (void*)&msgexit,
                                                        error)))
            return FALSE;

        g_signal_emit_by_name(console, "exited", msgexit.status);

        if (priv->localToStdoutLength == 0 &&
            priv->localToStderrLength == 0) {
            if (!do_console_rpc_set_state(console,
                                          GVIR_SANDBOX_CONSOLE_RPC_STATE_FINISHED,
                                          error))
                return FALSE;
        } else {
            if (!do_console_rpc_set_state(console,
                                          GVIR_SANDBOX_CONSOLE_RPC_STATE_STOPPING,
                                          error))
                return FALSE;
        }
        break;

    case GVIR_SANDBOX_PROTOCOL_PROC_QUIT:
    case GVIR_SANDBOX_PROTOCOL_PROC_STDIN:
    default:
        g_set_error(error, GVIR_SANDBOX_CONSOLE_RPC_ERROR, 0,
                    _("Unexpected rpc proc %u"),
                    pkt->header.proc);
        return FALSE;
    }
    return TRUE;
}


static gboolean
do_console_rpc_process_packet_rx(GVirSandboxConsoleRpc *console,
                                 GVirSandboxRPCPacket *pkt,
                                 GError **err)
{
    GVirSandboxConsoleRpcPrivate *priv = console->priv;

    //g_debug("Process rx state=%d len=%zu",
//            priv->state, pkt->bufferLength);

    switch (priv->state) {
    case GVIR_SANDBOX_CONSOLE_RPC_STATE_WAITING:
        if (pkt->buffer[0] == GVIR_SANDBOX_PROTOCOL_HANDSHAKE_SYNC) {
            if (!do_console_rpc_set_state(console,
                                          GVIR_SANDBOX_CONSOLE_RPC_STATE_SYNCING,
                                          err))
                return FALSE;
        } else {
            /* Try recv another byte */
            priv->rx = gvir_sandbox_rpcpacket_new(FALSE);
            priv->rx->bufferLength = 1; /* We need to recv a hanshake byte */
        }
        break;

    case GVIR_SANDBOX_CONSOLE_RPC_STATE_RUNNING:
        if (pkt->bufferLength == GVIR_SANDBOX_PROTOCOL_LEN_MAX) {
            if (!gvir_sandbox_rpcpacket_decode_length(pkt, err))
                return FALSE;
            /* Setup new packet to receive the payload */
            priv->rx = gvir_sandbox_rpcpacket_new(TRUE);
            memcpy(priv->rx, pkt, sizeof(*pkt));
        } else {
            if (!do_console_rpc_dispatch_proc(console, pkt, err))
                return FALSE;

            if (priv->state == GVIR_SANDBOX_CONSOLE_RPC_STATE_RUNNING &&
                priv->localToStdoutLength < GVIR_SANDBOX_CONSOLE_MAX_QUEUED_DATA &&
                priv->localToStderrLength < GVIR_SANDBOX_CONSOLE_MAX_QUEUED_DATA)
                priv->rx = gvir_sandbox_rpcpacket_new(TRUE);
        }
        break;

    case GVIR_SANDBOX_CONSOLE_RPC_STATE_STOPPING:
    case GVIR_SANDBOX_CONSOLE_RPC_STATE_FINISHED:
    case GVIR_SANDBOX_CONSOLE_RPC_STATE_INACTIVE:
    case GVIR_SANDBOX_CONSOLE_RPC_STATE_SYNCING:
    default:
        g_set_error(err, GVIR_SANDBOX_CONSOLE_RPC_ERROR, 0,
                    _("Got rx in unexpected state %d"), priv->state);
        return FALSE;
    }

    return TRUE;
}


static gboolean do_console_handshake_wait_tx_queue(gpointer opaque)
{
    GVirSandboxConsoleRpc *console = GVIR_SANDBOX_CONSOLE_RPC(opaque);
    GVirSandboxConsoleRpcPrivate *priv = console->priv;

    if (priv->state != GVIR_SANDBOX_CONSOLE_RPC_STATE_WAITING)
        return FALSE;

    if (priv->tx != NULL)
        return FALSE;

    priv->tx = gvir_sandbox_console_rpc_build_handshake_wait();
    do_console_rpc_update_events(console);

    return FALSE;
}


static gboolean
do_console_rpc_process_packet_tx(GVirSandboxConsoleRpc *console,
                                 GVirSandboxRPCPacket *pkt,
                                 GError **err)
{
    GVirSandboxConsoleRpcPrivate *priv = console->priv;

    switch (priv->state) {
    case GVIR_SANDBOX_CONSOLE_RPC_STATE_WAITING:
        g_debug("Schedule another wait");
        g_timeout_add(50,
                      do_console_handshake_wait_tx_queue,
                      console);
        break;
    case GVIR_SANDBOX_CONSOLE_RPC_STATE_SYNCING:
        if (pkt->buffer[0] == GVIR_SANDBOX_PROTOCOL_HANDSHAKE_WAIT) {
            g_debug("Schedule tx of sync packet");
            priv->tx = gvir_sandbox_console_rpc_build_handshake_sync();
        } else {
            if (!do_console_rpc_set_state(console,
                                          GVIR_SANDBOX_CONSOLE_RPC_STATE_RUNNING,
                                          err))
                return FALSE;
        }
        break;
    case GVIR_SANDBOX_CONSOLE_RPC_STATE_FINISHED:
        g_debug("Finished tx of last packet");
        do_console_rpc_close(console, NULL);
        break;

    case GVIR_SANDBOX_CONSOLE_RPC_STATE_RUNNING:
    case GVIR_SANDBOX_CONSOLE_RPC_STATE_STOPPING:
        /* no-op */
        break;

    case GVIR_SANDBOX_CONSOLE_RPC_STATE_INACTIVE:
    default:
        g_set_error(err, GVIR_SANDBOX_CONSOLE_RPC_ERROR, 0,
                    _("Got rx in unexpected state %d"), priv->state);
        return FALSE;
    }

    return TRUE;
}


static gboolean do_console_rpc_stream_readwrite(GVirStream *stream,
                                                GVirStreamIOCondition cond,
                                                gpointer opaque)
{
    GVirSandboxConsoleRpc *console = GVIR_SANDBOX_CONSOLE_RPC(opaque);
    GVirSandboxConsoleRpcPrivate *priv = console->priv;
    g_debug("Stream read write cond=%d state=%d rx=%p tx=%p",
            cond, priv->state, priv->rx, priv->tx);
    if (cond & GVIR_STREAM_IO_CONDITION_READABLE) {
        while (priv->rx) {
            GError *err = NULL;
            gssize ret = gvir_stream_receive
                (stream,
                 priv->rx->buffer + priv->rx->bufferOffset,
                 priv->rx->bufferLength - priv->rx->bufferOffset,
                 NULL,
                 &err);
            if (ret < 0) {
                if (err && err->code == G_IO_ERROR_WOULD_BLOCK) {
                    g_debug("Would block");
                    g_error_free(err);
                    break;
                } else {
                    g_debug("Error reading from stream");
                    do_console_rpc_close(console, err);
                    g_error_free(err);
                    goto cleanup;
                }
            }
            if (ret == 0) { /* EOF */
                goto done;
            }
            g_debug("offset=%zu length=%zu ret=%zd",
                    priv->rx->bufferOffset,
                    priv->rx->bufferLength,
                    ret);
            priv->rx->bufferOffset += ret;
            if (priv->rx->bufferOffset == priv->rx->bufferLength) {
                GVirSandboxRPCPacket *pkt = priv->rx;
                priv->rx = NULL;
                if (!do_console_rpc_process_packet_rx(console,
                                                      pkt,
                                                      &err)) {
                    gvir_sandbox_rpcpacket_free(pkt);
                    g_debug("Error process rx packet");
                    do_console_rpc_close(console, err);
                    g_error_free(err);
                    goto cleanup;
                }
                gvir_sandbox_rpcpacket_free(pkt);
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
            g_debug("Error writing to stream");
            do_console_rpc_close(console, err);
            g_error_free(err);
            goto cleanup;
        }

        priv->tx->bufferOffset += ret;
        if (priv->tx->bufferOffset == priv->tx->bufferLength) {
            GVirSandboxRPCPacket *pkt = priv->tx;
            priv->tx = NULL;
            if (!do_console_rpc_process_packet_tx(console,
                                                  pkt,
                                                  &err)) {
                gvir_sandbox_rpcpacket_free(pkt);
                g_debug("Error process tx packet");
                do_console_rpc_close(console, err);
                g_error_free(err);
                goto cleanup;
            }
            gvir_sandbox_rpcpacket_free(pkt);
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

    gssize ret = g_input_stream_read
        (G_INPUT_STREAM(localStdin),
         buf, MAX_IO,
         NULL, &err);
    if (ret < 0) {
        g_debug("Error reading from stdin");
        do_console_rpc_close(console, err);
        g_error_free(err);
        goto cleanup;
    }

    if (ret == 0)
        priv->localEOF = TRUE;
    else if (priv->allowEscape &&
             (buf[0] ==
              CONTROL(gvir_sandbox_console_get_escape(GVIR_SANDBOX_CONSOLE(console))))) {
        g_debug("Got console escape");
        do_console_rpc_close(console, err);
        goto cleanup;
    }

    if (!(priv->tx = gvir_sandbox_console_rpc_build_stdin(console, buf, ret, &err))) {
        g_debug("Failed to build stdin packet");
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
    g_debug("Stdout write %p %p",
            priv->localStdoutSource,
            priv->localToStdout);
    gssize ret = g_output_stream_write
        (G_OUTPUT_STREAM(localStdout),
         priv->localToStdout + priv->localToStdoutOffset,
         priv->localToStdoutLength - priv->localToStdoutOffset,
         NULL, &err);
    if (ret < 0) {
        g_debug("Failed write stdout");
        do_console_rpc_close(console, err);
        g_error_free(err);
        goto cleanup;
    }

    priv->localToStdoutOffset += ret;

    if (priv->localToStdoutOffset == priv->localToStdoutLength) {
        g_free(priv->localToStdout);
        priv->localToStdout = NULL;
        priv->localToStdoutOffset = priv->localToStdoutLength = 0;

        if (priv->state == GVIR_SANDBOX_CONSOLE_RPC_STATE_RUNNING &&
            !priv->rx &&
            priv->localToStderrLength < GVIR_SANDBOX_CONSOLE_MAX_QUEUED_DATA)
            priv->rx = gvir_sandbox_rpcpacket_new(TRUE);

        if (priv->state == GVIR_SANDBOX_CONSOLE_RPC_STATE_STOPPING &&
            priv->localToStderrLength == 0 &&
            !do_console_rpc_set_state(console,
                                      GVIR_SANDBOX_CONSOLE_RPC_STATE_FINISHED,
                                      &err)) {
            g_debug("Failed set finished state");
            do_console_rpc_close(console, err);
            g_error_free(err);
            goto cleanup;
        }
    } else if (0) {
        memmove(priv->localToStdout,
                priv->localToStdout + priv->localToStdoutOffset,
                priv->localToStdoutLength - priv->localToStdoutOffset);
        priv->localToStdoutLength -= priv->localToStdoutOffset;
        priv->localToStdoutOffset = 0;
        priv->localToStdout = g_renew(gchar,
                                      priv->localToStdout,
                                      priv->localToStdoutLength);
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
    g_debug("Stderr write %p %p",
            priv->localStderrSource,
            priv->localToStderr);
    gssize ret = g_output_stream_write
        (G_OUTPUT_STREAM(localStderr),
         priv->localToStderr,
         priv->localToStderrOffset,
         NULL, &err);
    if (ret < 0) {
        g_debug("Failed to write stderr");
        do_console_rpc_close(console, err);
        g_error_free(err);
        goto cleanup;
    }

    priv->localToStderrOffset += ret;

    if (priv->localToStderrOffset == priv->localToStderrLength) {
        g_free(priv->localToStderr);
        priv->localToStderr = NULL;
        priv->localToStderrOffset = priv->localToStderrLength = 0;

        if (priv->state == GVIR_SANDBOX_CONSOLE_RPC_STATE_RUNNING &&
            !priv->rx &&
            priv->localToStdoutLength < GVIR_SANDBOX_CONSOLE_MAX_QUEUED_DATA)
            priv->rx = gvir_sandbox_rpcpacket_new(TRUE);

        if (priv->state == GVIR_SANDBOX_CONSOLE_RPC_STATE_STOPPING &&
            priv->localToStdoutLength == 0 &&
            !do_console_rpc_set_state(console,
                                      GVIR_SANDBOX_CONSOLE_RPC_STATE_FINISHED,
                                      &err)) {
            g_debug("Failed set finished state");
            do_console_rpc_close(console, err);
            g_error_free(err);
            goto cleanup;
        }
    } else if (0) {
        memmove(priv->localToStderr,
                priv->localToStderr + priv->localToStderrOffset,
                priv->localToStderrLength - priv->localToStderrOffset);
        priv->localToStderrLength -= priv->localToStderrOffset;
        priv->localToStderrOffset = 0;
        priv->localToStderr = g_renew(gchar,
                                      priv->localToStderr,
                                      priv->localToStderrLength);
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

    if (priv->state != GVIR_SANDBOX_CONSOLE_RPC_STATE_INACTIVE) {
        g_set_error(error, GVIR_SANDBOX_CONSOLE_RPC_ERROR, 0, "%s",
                    _("Console is already attached to a stream"));
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

    if (!do_console_rpc_set_state(GVIR_SANDBOX_CONSOLE_RPC(console),
                                  GVIR_SANDBOX_CONSOLE_RPC_STATE_WAITING,
                                  error))
        goto cleanup;
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

    if (priv->state == GVIR_SANDBOX_CONSOLE_RPC_STATE_INACTIVE) {
        return TRUE;
#if 0
        g_set_error(error, GVIR_SANDBOX_CONSOLE_RPC_ERROR, 0, "%s",
                    _("ConsoleRpc is not attached to a stream"));
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

    priv->state = GVIR_SANDBOX_CONSOLE_RPC_STATE_INACTIVE;

    ret = TRUE;

//cleanup:
    return ret;
}
