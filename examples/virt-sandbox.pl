#!/usr/bin/perl

use strict;
use warnings;
use Glib::Object::Introspection;

Glib::Object::Introspection->setup(basename => 'GLib', version => '2.0', package => 'GLib');
Glib::Object::Introspection->setup(basename => 'Gtk', version => '3.0', package => 'Gtk');
Glib::Object::Introspection->setup(basename => 'LibvirtGObject', version => '1.0', package => 'LibvirtGObject');
Glib::Object::Introspection->setup(basename => 'LibvirtSandbox', version => '1.0', package => 'LibvirtSandbox');

LibvirtGObject::init_object_check(undef);

my $cfg = LibvirtSandbox::ConfigInteractive->new("sandbox");
if (int(@ARGV) > 0) {
    $cfg->set_command(@ARGV);
}
if (-t STDIN) {
    $cfg->set_tty(1);
}

my $conn = LibvirtGObject::Connection->new("qemu:///session");
$conn->open(undef);

my $ctxt = LibvirtSandbox::ContextInteractive->new($conn, $cfg);
$ctxt->start();

my $con = $ctxt->get_console();

sub closed {
    Gtk::main_quit();
}

$con->connect("closed", \&closed);
$con->attach_stdio();

Gtk::main();

eval {
    $con->detach();
};
eval {
    $ctxt->stop();
};
