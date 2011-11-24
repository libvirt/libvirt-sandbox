#!/usr/bin/gjs

const LibvirtGObject = imports.gi.LibvirtGObject;
const LibvirtSandbox = imports.gi.LibvirtSandbox;
const Gtk = imports.gi.Gtk;

LibvirtGObject.init_object_check(null, null);

var cfg = LibvirtSandbox.Config.new("sandbox");
var conn = LibvirtGObject.Connection.new("qemu:///session");
conn.open(null)

var ctxt = LibvirtSandbox.Context.new(conn, cfg);
ctxt.start();

var con = ctxt.get_console()

var closed = function(error) {
    Gtk.main_quit();
}

con.connect("closed", closed);

con.attach_stdio()

Gtk.main()

con.detach()

ctxt.stop();
