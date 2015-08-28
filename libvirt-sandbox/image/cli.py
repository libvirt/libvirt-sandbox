#!/usr/bin/python -Es
# -*- coding: utf-8 -*-
# Authors: Daniel P. Berrange <berrange@redhat.com>
#          Eren Yagdiran <erenyagdiran@gmail.com>
#
# Copyright (C) 2013 Red Hat, Inc.
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

if os.geteuid() == 0:
    default_template_dir = "/var/lib/libvirt/templates"
    default_image_dir = "/var/lib/libvirt/images"
else:
    default_template_dir = os.environ['HOME'] + "/.local/share/libvirt/templates"
    default_image_dir = os.environ['HOME'] + "/.local/share/libvirt/images"

debug = False
verbose = False

import importlib
def dynamic_source_loader(name):
    name = name[0].upper() + name[1:]
    modname = "libvirt_sandbox.image.sources." + name + "Source"
    mod = importlib.import_module(modname)
    classname = name + "Source"
    classimpl = getattr(mod, classname)
    return classimpl()

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

def get_url(server, path, headers):
    url = "https://" + server + path
    debug("  Fetching %s..." % url)
    req = urllib2.Request(url=url)

    if json:
        req.add_header("Accept", "application/json")

    for h in headers.keys():
        req.add_header(h, headers[h])

    return urllib2.urlopen(req)

def get_json(server, path, headers):
    try:
        res = get_url(server, path, headers)
        data = json.loads(res.read())
        debug("OK\n")
        return (data, res)
    except Exception, e:
        debug("FAIL %s\n" % str(e))
        raise

def save_data(server, path, headers, dest, checksum=None, datalen=None):
    try:
        res = get_url(server, path, headers)

        csum = None
        if checksum is not None:
            csum = hashlib.sha256()

        pattern = [".", "o", "O", "o"]
        patternIndex = 0
        donelen = 0

        with open(dest, "w") as f:
            while 1:
                buf = res.read(1024*64)
                if not buf:
                    break
                if csum is not None:
                    csum.update(buf)
                f.write(buf)

                if datalen is not None:
                    donelen = donelen + len(buf)
                    debug("\x1b[s%s (%5d Kb of %5d Kb)\x1b8" % (
                        pattern[patternIndex], (donelen/1024), (datalen/1024)
                    ))
                    patternIndex = (patternIndex + 1) % 4

        debug("\x1b[K")
        if csum is not None:
            csumstr = "sha256:" + csum.hexdigest()
            if csumstr != checksum:
                debug("FAIL checksum '%s' does not match '%s'" % (csumstr, checksum))
                os.remove(dest)
                raise IOError("Checksum '%s' for data does not match '%s'" % (csumstr, checksum))
        debug("OK\n")
        return res
    except Exception, e:
        debug("FAIL %s\n" % str(e))
        raise


def download_template(name, server, destdir):
    tag = "latest"

    offset = name.find(':')
    if offset != -1:
        tag = name[offset + 1:]
        name = name[0:offset]

    # First we must ask the index server about the image name. THe
    # index server will return an auth token we can use when talking
    # to the registry server. We need this token even when anonymous
    try:
        (data, res) = get_json(server, "/v1/repositories/" + name + "/images",
                               {"X-Docker-Token": "true"})
    except urllib2.HTTPError, e:
        raise ValueError(["Image '%s' does not exist" % name])

    registryserver = res.info().getheader('X-Docker-Endpoints')
    token = res.info().getheader('X-Docker-Token')
    checksums = {}
    for layer in data:
        pass
        # XXX Checksums here don't appear to match the data in
        # image download later. Find out what these are sums of
        #checksums[layer["id"]] = layer["checksum"]

    # Now we ask the registry server for the list of tags associated
    # with the image. Tags usually reflect some kind of version of
    # the image, but they aren't officially "versions". There is
    # always a "latest" tag which is the most recent upload
    #
    # We must pass in the auth token from the index server. This
    # token can only be used once, and we're given a cookie back
    # in return to use for later RPC calls.
    (data, res) = get_json(registryserver, "/v1/repositories/" + name + "/tags",
                           { "Authorization": "Token " + token })

    cookie = res.info().getheader('Set-Cookie')

    if not tag in data:
        raise ValueError(["Tag '%s' does not exist for image '%s'" % (tag, name)])
    imagetagid = data[tag]

    # Only base images are self-contained, most images reference one
    # or more parents, in a linear stack. Here we are getting the list
    # of layers for the image with the tag we used.
    (data, res) = get_json(registryserver, "/v1/images/" + imagetagid + "/ancestry",
                           { "Authorization": "Token " + token })

    if data[0] != imagetagid:
        raise ValueError(["Expected first layer id '%s' to match image id '%s'",
                          data[0], imagetagid])

    try:
        createdFiles = []
        createdDirs = []

        for layerid in data:
            templatedir = destdir + "/" + layerid
            if not os.path.exists(templatedir):
                os.mkdir(templatedir)
                createdDirs.append(templatedir)

            jsonfile = templatedir + "/template.json"
            datafile = templatedir + "/template.tar.gz"

            if not os.path.exists(jsonfile) or not os.path.exists(datafile):
                # The '/json' URL gives us some metadata about the layer
                res = save_data(registryserver, "/v1/images/" + layerid + "/json",
                                { "Authorization": "Token " + token }, jsonfile)
                createdFiles.append(jsonfile)
                layersize = int(res.info().getheader("Content-Length"))

                datacsum = None
                if layerid in checksums:
                    datacsum = checksums[layerid]

                # and the '/layer' URL is the actual payload, provided
                # as a tar.gz archive
                save_data(registryserver, "/v1/images/" + layerid + "/layer",
                          { "Authorization": "Token " + token }, datafile, datacsum, layersize)
                createdFiles.append(datafile)

        # Strangely the 'json' data for a layer doesn't include
        # its actual name, so we save that in a json file of our own
        index = {
            "name": name,
        }

        indexfile = destdir + "/" + imagetagid + "/index.json"
        with open(indexfile, "w") as f:
            f.write(json.dumps(index))
    except Exception, e:
        for f in createdFiles:
            try:
                os.remove(f)
            except:
                pass
        for d in createdDirs:
            try:
                os.rmdir(d)
            except:
                pass


def delete_template(name, destdir):
    imageusage = {}
    imageparent = {}
    imagenames = {}
    imagedirs = os.listdir(destdir)
    for imagetagid in imagedirs:
        indexfile = destdir + "/" + imagetagid + "/index.json"
        if os.path.exists(indexfile):
            with open(indexfile, "r") as f:
                index = json.load(f)
            imagenames[index["name"]] = imagetagid
        jsonfile = destdir + "/" + imagetagid + "/template.json"
        if os.path.exists(jsonfile):
            with open(jsonfile, "r") as f:
                template = json.load(f)

            parent = template.get("parent", None)
            if parent:
                if parent not in imageusage:
                    imageusage[parent] = []
                imageusage[parent].append(imagetagid)
                imageparent[imagetagid] = parent

    if not name in imagenames:
        raise ValueError(["Image %s does not exist locally" % name])

    imagetagid = imagenames[name]
    while imagetagid != None:
        debug("Remove %s\n" %  imagetagid)
        parent = imageparent.get(imagetagid, None)

        indexfile = destdir + "/" + imagetagid + "/index.json"
        if os.path.exists(indexfile):
            os.remove(indexfile)
        jsonfile = destdir + "/" + imagetagid + "/template.json"
        if os.path.exists(jsonfile):
            os.remove(jsonfile)
        datafile = destdir + "/" + imagetagid + "/template.tar.gz"
        if os.path.exists(datafile):
            os.remove(datafile)
        imagedir = destdir + "/" + imagetagid
        os.rmdir(imagedir)

        if parent:
            if len(imageusage[parent]) != 1:
                debug("Parent %s is shared\n" % parent)
                parent = None
        imagetagid = parent


def get_image_list(name, destdir):
    imageparent = {}
    imagenames = {}
    imagedirs = os.listdir(destdir)
    for imagetagid in imagedirs:
        indexfile = destdir + "/" + imagetagid + "/index.json"
        if os.path.exists(indexfile):
            with open(indexfile, "r") as f:
                index = json.load(f)
            imagenames[index["name"]] = imagetagid
        jsonfile = destdir + "/" + imagetagid + "/template.json"
        if os.path.exists(jsonfile):
            with open(jsonfile, "r") as f:
                template = json.load(f)

            parent = template.get("parent", None)
            if parent:
                imageparent[imagetagid] = parent

    if not name in imagenames:
        raise ValueError(["Image %s does not exist locally" % name])

    imagetagid = imagenames[name]
    imagelist = []
    while imagetagid != None:
        imagelist.append(imagetagid)
        parent = imageparent.get(imagetagid, None)
        imagetagid = parent

    return imagelist

def create_template(name, imagepath, format, destdir):
    if not format in ["qcow2"]:
        raise ValueError(["Unsupported image format %s" % format])

    imagelist = get_image_list(name, destdir)
    imagelist.reverse()

    parentImage = None
    for imagetagid in imagelist:
        templateImage = destdir + "/" + imagetagid + "/template." + format
        cmd = ["qemu-img", "create", "-f", "qcow2"]
        if parentImage is not None:
            cmd.append("-o")
            cmd.append("backing_fmt=qcow2,backing_file=%s" % parentImage)
        cmd.append(templateImage)
        if parentImage is None:
            cmd.append("10G")
        debug("Run %s\n" % " ".join(cmd))
        subprocess.call(cmd)
        parentImage = templateImage

def download(args):
    info("Downloading %s from %s to %s\n" % (args.template, default_index_server, default_template_dir))
    download_template(args.template, default_index_server, default_template_dir)

def delete(args):
    info("Deleting %s from %s\n" % (args.template, default_template_dir))
    delete_template(args.template, default_template_dir)

def create(args):
    info("Creating %s from %s in format %s\n" % (args.imagepath, args.template, args.format))
    create_template(args.template, args.imagepath, args.format, default_template_dir)

def requires_template(parser):
    parser.add_argument("template",
                        help=_("name of the template"))

def gen_download_args(subparser):
    parser = subparser.add_parser("download",
                                   help=_("Download template data"))
    requires_template(parser)
    parser.set_defaults(func=download)

def gen_delete_args(subparser):
    parser = subparser.add_parser("delete",
                                   help=_("Delete template data"))
    requires_template(parser)
    parser.set_defaults(func=delete)

def gen_create_args(subparser):
    parser = subparser.add_parser("create",
                                   help=_("Create image from template data"))
    requires_template(parser)
    parser.add_argument("imagepath",
                        help=_("path for image"))
    parser.add_argument("format",
                        help=_("format"))
    parser.set_defaults(func=create)

def main():
    parser = argparse.ArgumentParser(description='Sandbox Container Image Tool')

    subparser = parser.add_subparsers(help=_("commands"))
    gen_download_args(subparser)
    gen_delete_args(subparser)
    gen_create_args(subparser)

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
