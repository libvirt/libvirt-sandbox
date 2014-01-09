/*
 * libvirt-sandbox-builder-private.h: libvirt sandbox builder
 *
 * Copyright (C) 2014 Red Hat, Inc.
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
 * Author: Christophe Fergeau <cfergeau@redhat.com>
 */

#if !defined(__LIBVIRT_SANDBOX_H__) && !defined(LIBVIRT_SANDBOX_BUILD)
#error "Only <libvirt-sandbox/libvirt-sandbox.h> can be included directly."
#endif

#ifndef __LIBVIRT_SANDBOX_BUILDER_PRIVATE_H__
#define __LIBVIRT_SANDBOX_BUILDER_PRIVATE_H__

G_BEGIN_DECLS

void gvir_sandbox_builder_set_filterref(GVirSandboxBuilder *builder,
                                        GVirConfigDomainInterface *iface,
                                        GVirSandboxConfigNetworkFilterref *filterref);

G_END_DECLS

#endif /* __LIBVIRT_SANDBOX_BUILDER_PRIVATE_H__ */
