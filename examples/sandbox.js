#!/usr/bin/gjs

const LibvirtGObject = imports.gi.LibvirtGObject;
const LibvirtSandbox = imports.gi.LibvirtSandbox;

LibvirtGObject.init_object_check(null, null);

var cfg = LibvirtSandbox.Config.new("sandbox");
var conn = LibvirtGObject.Connection.new("qemu:///session");
conn.open(null)

conn.fetch_domains(null)

var ctxt = LibvirtSandbox.Context.new(conn, cfg);
ctxt.start();

ctxt.stop();
