#!/usr/bin/perl

use strict;
use warnings;
use Glib::Object::Introspection;

Glib::Object::Introspection->setup(basename => 'GLib', version => '2.0', package => 'GLib');
Glib::Object::Introspection->setup(basename => 'Gtk', version => '3.0', package => 'Gtk');
Glib::Object::Introspection->setup(basename => 'LibvirtGObject', version => '1.0', package => 'LibvirtGObject');
Glib::Object::Introspection->setup(basename => 'LibvirtSandbox', version => '1.0', package => 'LibvirtSandbox');

LibvirtGObject::init_object_check(undef);

sub do_quit {
    Gtk::main_quit();
}


print "Foo\n";

my $cfg = LibvirtSandbox::Config->new("sandbox");
my $conn = LibvirtGObject::Connection->new("qemu:///session");
$conn->open(undef);

my $ctxt = LibvirtSandbox::Context->new($conn, $cfg);
$ctxt->start();

my $con = $ctxt->get_console();

$con->connect("closed", \&do_quit);

$con->attach_stdio();

GLib::timeout_add(5000, \&do_quit);

Gtk::main();

$con->detach();


$ctxt->stop();
