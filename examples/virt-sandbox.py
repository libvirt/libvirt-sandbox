#!/usr/bin/python

from gi.repository import LibvirtGObject
from gi.repository import LibvirtSandbox
from gi.repository import GLib
from gi.repository import Gtk
import sys
import os

args = sys.argv[1:]

LibvirtGObject.init_object_check(None)

cfg = LibvirtSandbox.ConfigInteractive.new("sandbox")
if len(args) > 0:
    cfg.set_command(args)
if os.isatty(sys.stdin.fileno()):
    cfg.set_tty(True)

conn = LibvirtGObject.Connection.new("qemu:///session")
conn.open(None)

ctxt = LibvirtSandbox.ContextInteractive.new(conn, cfg)
ctxt.start()

con = ctxt.get_app_console()

def closed(obj, error):
    Gtk.main_quit()

con.connect("closed", closed)
con.attach_stdio()

Gtk.main()

try:
    con.detach()
except:
    pass
try:
    ctxt.stop()
except:
    pass
