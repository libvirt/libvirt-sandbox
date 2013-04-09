/*
 * libvirt-sandbox-rpcpacket.c: rpc helper APIs
 *
 * Copyright (C) 2010-2012 Red Hat, Inc.
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
 */

#include <config.h>

#include <stdlib.h>
#include <unistd.h>

#include <glib/gi18n.h>

#include "libvirt-sandbox-rpcpacket.h"

#define GVIR_SANDBOX_RPCPACKET_ERROR gvir_sandbox_rpcpacket_error_quark()

static GQuark
gvir_sandbox_rpcpacket_error_quark(void)
{
    return g_quark_from_static_string("gvir-sandbox-rpcpacket");
}

GVirSandboxRPCPacket *gvir_sandbox_rpcpacket_new(gboolean rxready)
{
    GVirSandboxRPCPacket *msg = g_new0(GVirSandboxRPCPacket, 1);

    if (rxready)
        msg->bufferLength = GVIR_SANDBOX_PROTOCOL_LEN_MAX;

    return msg;
}


void gvir_sandbox_rpcpacket_free(GVirSandboxRPCPacket *msg)
{
    if (!msg)
        return;

    g_free(msg);
}


gboolean gvir_sandbox_rpcpacket_decode_length(GVirSandboxRPCPacket *msg,
                                              GError **error)
{
    XDR xdr;
    unsigned int len;
    gboolean ret = FALSE;

    xdrmem_create(&xdr, msg->buffer,
                  msg->bufferLength, XDR_DECODE);
    if (!xdr_u_int(&xdr, &len)) {
        g_set_error(error, GVIR_SANDBOX_RPCPACKET_ERROR, 0,
                    "%s", _("Unable to decode message length"));
        goto cleanup;
    }
    msg->bufferOffset = xdr_getpos(&xdr);

    if (len < GVIR_SANDBOX_PROTOCOL_LEN_MAX) {
        g_set_error(error, GVIR_SANDBOX_RPCPACKET_ERROR, 0,
                    _("packet %u bytes received from server too small, want %u"),
                    len, GVIR_SANDBOX_PROTOCOL_LEN_MAX);
        goto cleanup;
    }

    /* Length includes length word - adjust to real length to read. */
    len -= GVIR_SANDBOX_PROTOCOL_LEN_MAX;

    if (len > GVIR_SANDBOX_PROTOCOL_PACKET_MAX) {
        g_set_error(error, GVIR_SANDBOX_RPCPACKET_ERROR, 0,
                    _("packet %u bytes received from server too large, want %d"),
                    len, GVIR_SANDBOX_PROTOCOL_PACKET_MAX);
        goto cleanup;
    }

    /* Extend our declared buffer length and carry
       on reading the header + payload */
    msg->bufferLength += len;

    ret = TRUE;

cleanup:
    xdr_destroy(&xdr);
    return ret;
}


/*
 * @msg: the complete incoming message, whose header to decode
 *
 * Decodes the header part of the message, but does not
 * validate the decoded fields in the header. It expects
 * bufferLength to refer to length of the data packet. Upon
 * return bufferOffset will refer to the amount of the packet
 * consumed by decoding of the header.
 *
 * returns 0 if successfully decoded, -1 upon fatal error
 */
gboolean gvir_sandbox_rpcpacket_decode_header(GVirSandboxRPCPacket *msg,
                                              GError **error)
{
    XDR xdr;
    gboolean ret = FALSE;

    msg->bufferOffset = GVIR_SANDBOX_PROTOCOL_LEN_MAX;

    /* Parse the header. */
    xdrmem_create(&xdr,
                  msg->buffer + msg->bufferOffset,
                  msg->bufferLength - msg->bufferOffset,
                  XDR_DECODE);

    if (!xdr_GVirSandboxProtocolHeader(&xdr, &msg->header)) {
        g_set_error(error, GVIR_SANDBOX_RPCPACKET_ERROR, 0,
                    "%s", _("Unable to decode message header"));
        goto cleanup;
    }

    msg->bufferOffset += xdr_getpos(&xdr);

    ret = TRUE;

cleanup:
    xdr_destroy(&xdr);
    return ret;
}


/*
 * @msg: the outgoing message, whose header to encode
 *
 * Encodes the length word and header of the message, setting the
 * message offset ready to encode the payload. Leaves space
 * for the length field later. Upon return bufferLength will
 * refer to the total available space for message, while
 * bufferOffset will refer to current space used by header
 *
 * returns 0 if successfully encoded, -1 upon fatal error
 */
gboolean gvir_sandbox_rpcpacket_encode_header(GVirSandboxRPCPacket *msg,
                                              GError **error)
{
    XDR xdr;
    gboolean ret = FALSE;
    unsigned int len = 0;

    msg->bufferLength = sizeof(msg->buffer);
    msg->bufferOffset = 0;

    /* Format the header. */
    xdrmem_create(&xdr,
                  msg->buffer,
                  msg->bufferLength,
                  XDR_ENCODE);

    /* The real value is filled in shortly */
    if (!xdr_u_int(&xdr, &len)) {
        g_set_error(error, GVIR_SANDBOX_RPCPACKET_ERROR, 0,
                    "%s", _("Unable to encode message length"));
        goto cleanup;
    }

    if (!xdr_GVirSandboxProtocolHeader(&xdr, &msg->header)) {
        g_set_error(error, GVIR_SANDBOX_RPCPACKET_ERROR, 0,
                    "%s", _("Unable to encode message header"));
        goto cleanup;
    }

    len = xdr_getpos(&xdr);
    xdr_setpos(&xdr, 0);

    /* Fill in current length - may be re-written later
     * if a payload is added
     */
    if (!xdr_u_int(&xdr, &len)) {
        g_set_error(error, GVIR_SANDBOX_RPCPACKET_ERROR, 0,
                    "%s", _("Unable to re-encode message length"));
        goto cleanup;
    }

    msg->bufferOffset += len;

    ret = TRUE;

cleanup:
    xdr_destroy(&xdr);
    return ret;
}


gboolean gvir_sandbox_rpcpacket_encode_payload_msg(GVirSandboxRPCPacket *msg,
                                                   xdrproc_t filter,
                                                   void *data,
                                                   GError **error)
{
    XDR xdr;
    unsigned int msglen;

    /* Serialise payload of the message. This assumes that
     * GVirSandboxRPCPacketEncodeHeader has already been run, so
     * just appends to that data */
    xdrmem_create(&xdr, msg->buffer + msg->bufferOffset,
                  msg->bufferLength - msg->bufferOffset, XDR_ENCODE);

    if (!(*filter)(&xdr, data)) {
        g_set_error(error, GVIR_SANDBOX_RPCPACKET_ERROR, 0,
                    "%s", _("Unable to encode message payload"));
        goto error;
    }

    /* Get the length stored in buffer. */
    msg->bufferOffset += xdr_getpos(&xdr);
    xdr_destroy(&xdr);

    /* Re-encode the length word. */
    xdrmem_create(&xdr, msg->buffer, GVIR_SANDBOX_PROTOCOL_LEN_MAX, XDR_ENCODE);
    msglen = msg->bufferOffset;
    if (!xdr_u_int(&xdr, &msglen)) {
        g_set_error(error, GVIR_SANDBOX_RPCPACKET_ERROR, 0,
                    "%s", _("Unable to encode message length"));
        goto error;
    }
    xdr_destroy(&xdr);

    msg->bufferLength = msg->bufferOffset;
    msg->bufferOffset = 0;
    return TRUE;

error:
    xdr_destroy(&xdr);
    return FALSE;
}


gboolean gvir_sandbox_rpcpacket_decode_payload_msg(GVirSandboxRPCPacket *msg,
                                                   xdrproc_t filter,
                                                   void *data,
                                                   GError **error)
{
    XDR xdr;

    /* Deserialise payload of the message. This assumes that
     * gvir_sandbox_rpcpacketDecodeHeader has already been run, so
     * just start from after that data */
    xdrmem_create(&xdr, msg->buffer + msg->bufferOffset,
                  msg->bufferLength - msg->bufferOffset, XDR_DECODE);

    if (!(*filter)(&xdr, data)) {
        g_set_error(error, GVIR_SANDBOX_RPCPACKET_ERROR, 0,
                    "%s", _("Unable to decode message payload"));
        goto error;
    }

    /* Get the length stored in buffer. */
    msg->bufferOffset += xdr_getpos(&xdr);
    xdr_destroy(&xdr);
    return TRUE;

error:
    xdr_destroy(&xdr);
    return FALSE;
}


gboolean gvir_sandbox_rpcpacket_encode_payload_raw(GVirSandboxRPCPacket *msg,
                                                   const char *data,
                                                   gsize len,
                                                   GError **error)
{
    XDR xdr;
    unsigned int msglen;

    if ((msg->bufferLength - msg->bufferOffset) < len) {
        g_set_error(error, GVIR_SANDBOX_RPCPACKET_ERROR, 0,
                    _("Raw data too long to send (%zu bytes needed, %zu bytes available)"),
                    len, (msg->bufferLength - msg->bufferOffset));
        return FALSE;
    }

    memcpy(msg->buffer + msg->bufferOffset, data, len);
    msg->bufferOffset += len;

    /* Re-encode the length word. */
    xdrmem_create(&xdr, msg->buffer, GVIR_SANDBOX_PROTOCOL_LEN_MAX, XDR_ENCODE);
    msglen = msg->bufferOffset;
    if (!xdr_u_int(&xdr, &msglen)) {
        g_set_error(error, GVIR_SANDBOX_RPCPACKET_ERROR, 0,
                    "%s", _("Unable to encode message length"));
        goto error;
    }
    xdr_destroy(&xdr);

    msg->bufferLength = msg->bufferOffset;
    msg->bufferOffset = 0;
    return TRUE;

error:
    xdr_destroy(&xdr);
    return FALSE;
}


gboolean gvir_sandbox_rpcpacket_encode_payload_empty(GVirSandboxRPCPacket *msg,
                                                     GError **error)
{
    XDR xdr;
    unsigned int msglen;

    /* Re-encode the length word. */
    xdrmem_create(&xdr, msg->buffer, GVIR_SANDBOX_PROTOCOL_LEN_MAX, XDR_ENCODE);
    msglen = msg->bufferOffset;
    if (!xdr_u_int(&xdr, &msglen)) {
        g_set_error(error, GVIR_SANDBOX_RPCPACKET_ERROR, 0,
                    "%s", _("Unable to encode message length"));
        goto error;
    }
    xdr_destroy(&xdr);

    msg->bufferLength = msg->bufferOffset;
    msg->bufferOffset = 0;
    return TRUE;

error:
    xdr_destroy(&xdr);
    return FALSE;
}
