#!/usr/bin/python

from gi.repository import LibvirtGObject
from gi.repository import LibvirtSandbox

LibvirtGObject.init_object_check(None)

cfg = LibvirtSandbox.Config.new("sandbox")
conn = LibvirtGObject.Connection.new("qemu:///session")
conn.open(None)

ctxt = LibvirtSandbox.Context.new(conn, cfg)
ctxt.start()
ctxt.stop()
