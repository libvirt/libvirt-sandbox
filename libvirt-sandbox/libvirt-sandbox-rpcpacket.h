/*
 * libvirt-sandbox-rpcpacket.h: rpc helper APIs
 *
 * Copyright (C) 2010-2011 Red Hat, Inc.
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

#ifndef __LIBVIRT_SANDBOX_RPC_H__
# define __LIBVIRT_SANDBOX_RPC_H__

# include "glib.h"

# include "libvirt-sandbox-protocol.h"

typedef struct _GVirSandboxRPCPacket GVirSandboxRPCPacket;

/* Never allocate this (huge) buffer on the stack. Always
 * use GVirSandboxRPCPacketNew() to allocate on the heap
 */
struct _GVirSandboxRPCPacket {
    char buffer[GVIR_SANDBOX_PROTOCOL_PACKET_MAX + GVIR_SANDBOX_PROTOCOL_LEN_MAX];
    gsize bufferLength;
    gsize bufferOffset;

    GVirSandboxProtocolHeader header;
};


GVirSandboxRPCPacket *gvir_sandbox_rpcpacket_new(gboolean rxready);

void gvir_sandbox_rpcpacket_free(GVirSandboxRPCPacket *pkt);


gboolean gvir_sandbox_rpcpacket_decode_length(GVirSandboxRPCPacket *pkt,
                                              GError **error);
gboolean gvir_sandbox_rpcpacket_decode_header(GVirSandboxRPCPacket *pkt,
                                              GError **error);
gboolean gvir_sandbox_rpcpacket_decode_payload_msg(GVirSandboxRPCPacket *pkt,
                                                   xdrproc_t filter,
                                                   void *data,
                                                   GError **error);

gboolean gvir_sandbox_rpcpacket_encode_header(GVirSandboxRPCPacket *pkt,
                                              GError **error);
gboolean gvir_sandbox_rpcpacket_encode_payload_msg(GVirSandboxRPCPacket *pkt,
                                                   xdrproc_t filter,
                                                   void *data,
                                                   GError **error);
gboolean gvir_sandbox_rpcpacket_encode_payload_raw(GVirSandboxRPCPacket *msg,
                                                   const char *buf,
                                                   size_t len,
                                                   GError **error);
gboolean gvir_sandbox_rpcpacket_encode_payload_empty(GVirSandboxRPCPacket *msg,
                                                     GError **error);

#endif /* __VIR_NET_MESSAGE_H__ */
