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

#ifndef __LIBVIRT_SANDBOX_CONFIG_ALL_H__
#define __LIBVIRT_SANDBOX_CONFIG_ALL_H__

/* External include */
#include <libvirt-gconfig/libvirt-gconfig.h>
#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

/* Local includes */
#include <libvirt-sandbox/libvirt-sandbox-util.h>
#include <libvirt-sandbox/libvirt-sandbox-config-disk.h>
#include <libvirt-sandbox/libvirt-sandbox-config-mount.h>
#include <libvirt-sandbox/libvirt-sandbox-config-mount-file.h>
#include <libvirt-sandbox/libvirt-sandbox-config-mount-host-bind.h>
#include <libvirt-sandbox/libvirt-sandbox-config-mount-host-image.h>
#include <libvirt-sandbox/libvirt-sandbox-config-mount-guest-bind.h>
#include <libvirt-sandbox/libvirt-sandbox-config-mount-ram.h>
#include <libvirt-sandbox/libvirt-sandbox-config-network-address.h>
#include <libvirt-sandbox/libvirt-sandbox-config-network-filterref-parameter.h>
#include <libvirt-sandbox/libvirt-sandbox-config-network-filterref.h>
#include <libvirt-sandbox/libvirt-sandbox-config-network-route.h>
#include <libvirt-sandbox/libvirt-sandbox-config-network.h>
#include <libvirt-sandbox/libvirt-sandbox-config.h>
#include <libvirt-sandbox/libvirt-sandbox-config-initrd.h>
#include <libvirt-sandbox/libvirt-sandbox-config-interactive.h>
#include <libvirt-sandbox/libvirt-sandbox-config-service.h>
#include <libvirt-sandbox/libvirt-sandbox-config-service-systemd.h>
#include <libvirt-sandbox/libvirt-sandbox-config-service-generic.h>

#endif /* __LIBVIRT_SANDBOX_CONFIG_ALL_H__ */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
