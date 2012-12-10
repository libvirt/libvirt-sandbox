#!/usr/bin/python

from gi.repository import LibvirtGObject
from gi.repository import LibvirtSandbox
from gi.repository import GLib
from gi.repository import Gtk

LibvirtGObject.init_object_check(None)

config = LibvirtSandbox.Config.new("sandbox")
config.set_tty(True)
net = LibvirtSandbox.ConfigNetwork().new()
net.set_dhcp(True)
config.add_network(net)
config.set_command(["/sbin/httpd", "-some", "-arg"])

# Assuming '/tmp/privatestuff' contains a fake /etc for the
# guest

tmp = LibvirtSandbox.ConfigMount.new("/tmp")
# This is a host path
tmp.set_root("/tmp/privatestuff")
config.add_host_mount(tmp)

etc = LibvirtSandbox.ConfigMount.new("/etc")
# This is a guest path
etc.set_root("/tmp/etc")
config.add_bind_mount(etc)


#conn=LibvirtGObject.Connection.new("qemu:///session")
conn=LibvirtGObject.Connection.new("lxc:///")
conn.open(None)

context = LibvirtSandbox.Context.new(conn, config)
context.start()

def closed(obj, error):
    Gtk.main_quit()


console = context.get_console()
console.connect("closed", closed)
console.attach_stdio()

Gtk.main()

try:
    console.detach()
except:
    pass
try:
    context.stop()
except:
    pass
