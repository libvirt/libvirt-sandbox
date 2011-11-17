#!/usr/bin/perl

use strict;
use warnings;
use Glib::Object::Introspection;

Glib::Object::Introspection->setup(basename => 'LibvirtGObject', version => '1.0', package => 'LibvirtGObject');
Glib::Object::Introspection->setup(basename => 'LibvirtSandbox', version => '1.0', package => 'LibvirtSandbox');

LibvirtGObject::init_object_check(undef);

my $cfg = LibvirtSandbox::Config->new("sandbox");
my $conn = LibvirtGObject::Connection->new("qemu:///session");
$conn->open(undef);

$conn->fetch_domains(undef);

my $ctxt = LibvirtSandbox::Context->new($conn, $cfg);
$ctxt->start();
$ctxt->stop();
