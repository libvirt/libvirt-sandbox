#!/usr/bin/python

from gi.repository import LibvirtGObject
from gi.repository import LibvirtSandbox
from gi.repository import GLib
from gi.repository import Gtk
import sys

args = sys.argv[1:]

LibvirtGObject.init_object_check(None)

cfg = LibvirtSandbox.Config.new("sandbox")
cfg.set_command(args)
cfg.set_tty(True)
conn = LibvirtGObject.Connection.new("qemu:///session")
conn.open(None)

ctxt = LibvirtSandbox.Context.new(conn, cfg)
ctxt.start()

con = ctxt.get_console()

def closed(obj, error):
    Gtk.main_quit()

con.connect("closed", closed)
con.attach_stdio()

Gtk.main()

try:
    con.detach()
    ctxt.stop()
except:
    pass
finally:
    pass
