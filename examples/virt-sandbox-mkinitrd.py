#!/usr/bin/python

from gi.repository import LibvirtGObject
from gi.repository import LibvirtSandbox

LibvirtGObject.init_object_check(None)

cfg = LibvirtSandbox.ConfigInitrd.new()
cfg.set_init("/sbin/busybox")
cfg.add_module("virtio-rng.ko")
cfg.add_module("virtio_net.ko")

bld = LibvirtSandbox.BuilderInitrd.new()
bld.construct(cfg, "/tmp/demo.initrd")
