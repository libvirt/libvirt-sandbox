/*
 * libvirt-sandbox.h: libvirt sandbox integration
[ *
 * Copyright (C) 2010 Red Hat, Inc.
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

#ifndef __LIBVIRT_SANDBOX_H__
#define __LIBVIRT_SANDBOX_H__

/* External includes */
#include <libvirt-gobject/libvirt-gobject.h>

/* Local includes */
#include <libvirt-sandbox/libvirt-sandbox-main.h>
#include <libvirt-sandbox/libvirt-sandbox-util.h>
#include <libvirt-sandbox/libvirt-sandbox-enum-types.h>
#include <libvirt-sandbox/libvirt-sandbox-config-mount.h>
#include <libvirt-sandbox/libvirt-sandbox-config-mount-file.h>
#include <libvirt-sandbox/libvirt-sandbox-config-mount-host-bind.h>
#include <libvirt-sandbox/libvirt-sandbox-config-mount-host-image.h>
#include <libvirt-sandbox/libvirt-sandbox-config-mount-guest-bind.h>
#include <libvirt-sandbox/libvirt-sandbox-config-mount-ram.h>
#include <libvirt-sandbox/libvirt-sandbox-config-network-address.h>
#include <libvirt-sandbox/libvirt-sandbox-config-network-route.h>
#include <libvirt-sandbox/libvirt-sandbox-config-network.h>
#include <libvirt-sandbox/libvirt-sandbox-config.h>
#include <libvirt-sandbox/libvirt-sandbox-config-initrd.h>
#include <libvirt-sandbox/libvirt-sandbox-config-interactive.h>
#include <libvirt-sandbox/libvirt-sandbox-config-service.h>
#include <libvirt-sandbox/libvirt-sandbox-config-service-systemd.h>
#include <libvirt-sandbox/libvirt-sandbox-config-service-generic.h>
#include <libvirt-sandbox/libvirt-sandbox-builder.h>
#include <libvirt-sandbox/libvirt-sandbox-builder-initrd.h>
#include <libvirt-sandbox/libvirt-sandbox-builder-machine.h>
#include <libvirt-sandbox/libvirt-sandbox-builder-container.h>
#include <libvirt-sandbox/libvirt-sandbox-console.h>
#include <libvirt-sandbox/libvirt-sandbox-console-raw.h>
#include <libvirt-sandbox/libvirt-sandbox-console-rpc.h>
#include <libvirt-sandbox/libvirt-sandbox-context.h>
#include <libvirt-sandbox/libvirt-sandbox-context-interactive.h>
#include <libvirt-sandbox/libvirt-sandbox-context-service.h>

#endif /* __LIBVIRT_SANDBOX_H__ */
