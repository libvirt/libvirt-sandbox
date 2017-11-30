#!/usr/bin/env python3

from gi.repository import LibvirtGObject
from gi.repository import LibvirtSandbox
from gi.repository import GLib
from gi.repository import Gtk

LibvirtGObject.init_object_check(None)

config = LibvirtSandbox.Config.new("sandbox")
config.set_tty(True)

#conn=LibvirtGObject.Connection.new("qemu:///session")
conn=LibvirtGObject.Connection.new("lxc:///")
conn.open(None)

context = LibvirtSandbox.Context.new(conn, config)
context.attach()

def closed(obj, error):
    Gtk.main_quit()

console = context.get_shell_console()
console.connect("closed", closed)
console.attach_stdio()

Gtk.main()

try:
    console.detach()
except:
    pass
