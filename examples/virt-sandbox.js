#!/usr/bin/gjs

const LibvirtGObject = imports.gi.LibvirtGObject;
const LibvirtSandbox = imports.gi.LibvirtSandbox;
const Gtk = imports.gi.Gtk;

LibvirtGObject.init_object_check(null, null);

var cfg = LibvirtSandbox.ConfigInteractive.new("sandbox");
/* XXX how to set argv & check if stdin is a tty ? */
cfg.set_tty(true);

var conn = LibvirtGObject.Connection.new("qemu:///session");
conn.open(null)

var ctxt = LibvirtSandbox.ContextInteractive.new(conn, cfg);
ctxt.start();

var con = ctxt.get_console()

var closed = function(error) {
    Gtk.main_quit();
}

con.connect("closed", closed);

con.attach_stdio()

Gtk.main()

try {
    con.detach()
} catch (err) {}

try {
    ctxt.stop();
} catch (err) {}
