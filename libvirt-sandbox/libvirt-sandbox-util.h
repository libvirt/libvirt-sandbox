/*
 * libvirt-sandbox-util.h: libvirt sandbox utility code
 *
 * Copyright (C) 2013 Red Hat, Inc.
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
#error "Only <libvirt-gsandbox/libvirt-gsandbox.h> can be included directly."
#endif

#ifndef __LIBVIRT_SANDBOX_UTIL_H__
#define __LIBVIRT_SANDBOX_UTIL_H__

G_BEGIN_DECLS

/**
 * LIBVIRT_SANDBOX_CLASS_PADDING: (skip)
 */
#define LIBVIRT_SANDBOX_CLASS_PADDING 20

G_END_DECLS

#endif /* __LIBVIRT_SANDBOX_UTIL_H__ */
