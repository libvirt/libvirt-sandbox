#!/usr/bin/python -Es
# -*- coding: utf-8 -*-
# Authors: Daniel P. Berrange <berrange@redhat.com>
#          Eren Yagdiran <erenyagdiran@gmail.com>
#
# Copyright (C) 2013-2015 Red Hat, Inc.
# Copyright (C) 2015 Universitat Politècnica de Catalunya.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#

import argparse
import gettext
import hashlib
import json
import os
import os.path
import shutil
import sys
import urllib2
import subprocess
import random
import string

from libvirt_sandbox.image import template

if os.geteuid() == 0:
    default_template_dir = "/var/lib/libvirt/templates"
    default_image_dir = "/var/lib/libvirt/images"
else:
    default_template_dir = os.environ['HOME'] + "/.local/share/libvirt/templates"
    default_image_dir = os.environ['HOME'] + "/.local/share/libvirt/images"

debug = False
verbose = False

gettext.bindtextdomain("libvirt-sandbox", "/usr/share/locale")
gettext.textdomain("libvirt-sandbox")
try:
    gettext.install("libvirt-sandbox",
                    localedir="/usr/share/locale",
                    unicode=False,
                    codeset = 'utf-8')
except IOError:
    import __builtin__
    __builtin__.__dict__['_'] = unicode


def debug(msg):
    sys.stderr.write(msg)

def info(msg):
    sys.stdout.write(msg)

def delete(args):
    tmpl = template.Template.from_uri(args.template)
    source = tmpl.get_source_impl()
    source.delete_template(template=tmpl,
                           templatedir=args.template_dir)

def create(args):
    tmpl = template.Template.from_uri(args.template)
    source = tmpl.get_source_impl()
    source.create_template(template=tmpl,
                           templatedir=args.template_dir,
                           connect=args.connect)

def run(args):
    if args.connect is not None:
        check_connect(args.connect)

    tmpl = template.Template.from_uri(args.template)
    source = tmpl.get_source_impl()

    # Create the template image if needed
    if not source.has_template(tmpl, args.template_dir):
        create(args)

    name = args.name
    if name is None:
        randomid = ''.join(random.choice(string.lowercase) for i in range(10))
        name = tmpl.path[1:] + ":" + randomid

    diskfile = source.get_disk(template=tmpl,
                               templatedir=args.template_dir,
                               imagedir=args.image_dir,
                               sandboxname=name)

    commandToRun = source.get_command(tmpl, args.template_dir, args.args)
    if len(commandToRun) == 0:
        commandToRun = ["/bin/sh"]
    cmd = ['virt-sandbox', '--name', name]
    if args.connect is not None:
        cmd.append("-c")
        cmd.append(args.connect)
    params = ['-m','host-image:/=%s,format=qcow2' % diskfile]

    networkArgs = args.network
    if networkArgs is not None:
        params.append('-N')
        params.append(networkArgs)

    allEnvs = source.get_env(tmpl, args.template_dir)
    envArgs = args.env
    if envArgs is not None:
        allEnvs = allEnvs + envArgs
    for env in allEnvs:
        envsplit = env.split("=")
        envlen = len(envsplit)
        if envlen == 2:
            params.append("--env")
            params.append(env)
        else:
            pass

    cmd = cmd + params + ['--'] + commandToRun
    subprocess.call(cmd)
    os.unlink(diskfile)
    source.post_run(tmpl, args.template_dir, name)

def requires_template(parser):
    parser.add_argument("template",
                        help=_("URI of the template"))

def requires_name(parser):
    parser.add_argument("-n","--name",
                        help=_("Name of the running sandbox"))

def check_connect(connectstr):
    supportedDrivers = ['lxc:///','qemu:///session','qemu:///system']
    if not connectstr in supportedDrivers:
        raise ValueError("URI '%s' is not supported by virt-sandbox-image" % connectstr)
    return True

def requires_connect(parser):
    parser.add_argument("-c","--connect",
                        help=_("Connect string for libvirt"))

def requires_template_dir(parser):
    global default_template_dir
    parser.add_argument("-t","--template-dir",
                        default=default_template_dir,
                        help=_("Template directory for saving templates"))

def requires_image_dir(parser):
    global default_image_dir
    parser.add_argument("-I","--image-dir",
                        default=default_image_dir,
                        help=_("Image directory for saving images"))

def gen_command_parser(subparser, name, helptext):
    parser = subparser.add_parser(
        name, help=helptext,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""

Example supported URI formats:

  docker:///ubuntu?tag=15.04
  docker://username:password@index.docker.io/private/image
  docker://registry.access.redhat.com/rhel6
""")
    return parser

def gen_delete_args(subparser):
    parser = gen_command_parser(subparser, "delete",
                                _("Delete template data"))
    requires_template(parser)
    requires_template_dir(parser)
    parser.set_defaults(func=delete)

def gen_create_args(subparser):
    parser = gen_command_parser(subparser, "create",
                                _("Create image from template data"))
    requires_template(parser)
    requires_connect(parser)
    requires_template_dir(parser)
    parser.set_defaults(func=create)

def gen_run_args(subparser):
    parser = gen_command_parser(subparser, "run",
                                _("Run an already built image"))
    requires_name(parser)
    requires_template(parser)
    requires_connect(parser)
    requires_template_dir(parser)
    requires_image_dir(parser)
    parser.add_argument("args",
                        nargs=argparse.REMAINDER,
                        help=_("command arguments to run"))
    parser.add_argument("-N","--network",
                        help=_("Network params for running template"))
    parser.add_argument("-e","--env",action="append",
                        help=_("Environment params for running template"))

    parser.set_defaults(func=run)

def main():
    parser = argparse.ArgumentParser(description="Sandbox Container Image Tool")

    subparser = parser.add_subparsers(help=_("commands"))
    gen_delete_args(subparser)
    gen_create_args(subparser)
    gen_run_args(subparser)

    try:
        args = parser.parse_args()
        args.func(args)
        sys.exit(0)
    except KeyboardInterrupt, e:
        sys.exit(0)
    except ValueError, e:
        for line in e:
            for l in line:
                sys.stderr.write("%s: %s\n" % (sys.argv[0], l))
        sys.stderr.flush()
        sys.exit(1)
    except IOError, e:
        sys.stderr.write("%s: %s: %s\n" % (sys.argv[0], e.filename, e.reason))
        sys.stderr.flush()
        sys.exit(1)
    except OSError, e:
        sys.stderr.write("%s: %s\n" % (sys.argv[0], e))
        sys.stderr.flush()
        sys.exit(1)
    except Exception, e:
        print e.message
        sys.exit(1)
