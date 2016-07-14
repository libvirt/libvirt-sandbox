#!/usr/bin/python
# -*- coding: utf-8 -*-
#
# Copyright (C) 2015 Universitat Politècnica de Catalunya.
# Copyright (C) 2015 Red Hat, Inc.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
#
# Author: Eren Yagdiran <erenyagdiran@gmail.com>
#

import urllib2
import sys
import json
import traceback
import os
import subprocess
import shutil
import urlparse
import hashlib

from . import base

class DockerConfParser():

    def __init__(self,jsonfile):
        with open(jsonfile) as json_file:
            self.json_data = json.load(json_file)
    def getCommand(self):
        return self.json_data['config']['Cmd']
    def getEntrypoint(self):
        return self.json_data['config']['Entrypoint']
    def getEnvs(self):
        lst = self.json_data['config']['Env']
        if lst is not None and isinstance(lst,list):
          return lst
        else:
          return []

class DockerSource(base.Source):

    def _check_cert_validate(self):
        major = sys.version_info.major
        SSL_WARNING = "SSL certificates couldn't be validated by default. You need to have 2.7.9/3.4.3 or higher"
        SSL_WARNING +="\nSee https://bugs.python.org/issue22417\n"
        py2_7_9_hexversion = 34015728
        py3_4_3_hexversion = 50594800
        if  (major == 2 and sys.hexversion < py2_7_9_hexversion) or (major == 3 and sys.hexversion < py3_4_3_hexversion):
            sys.stderr.write(SSL_WARNING)

    def _was_downloaded(self, template, templatedir):
        try:
            self._get_image_list(template, templatedir)
            return True
        except Exception:
            return False


    def has_template(self, template, templatedir):
        try:
            configfile, diskfile = self._get_template_data(template, templatedir)
            return os.path.exists(diskfile)
        except Exception:
            return False


    def download_template(self, template, templatedir):
        self._check_cert_validate()

        try:
            (data, res) = self._get_json(template,
                                         None,
                                         "/v1/repositories" + template.path + "/images",
                                         {"X-Docker-Token": "true"})
        except urllib2.HTTPError, e:
            raise ValueError(["Image '%s' does not exist" % template])

        registryendpoint = res.info().getheader('X-Docker-Endpoints')
        token = res.info().getheader('X-Docker-Token')
        checksums = {}
        for layer in data:
            pass

        headers = {}
        if token is not None:
            headers["Authorization"] = "Token " + token
        (data, res) = self._get_json(template,
                                     registryendpoint,
                                     "/v1/repositories" + template.path + "/tags",
                                     headers)

        cookie = res.info().getheader('Set-Cookie')

        tag = template.params.get("tag", "latest")
        if not tag in data:
            raise ValueError(["Tag '%s' does not exist for image '%s'" % (tag, template)])
        imagetagid = data[tag]

        (data, res) = self._get_json(template,
                                     registryendpoint,
                                     "/v1/images/" + imagetagid + "/ancestry",
                                     headers)

        if data[0] != imagetagid:
            raise ValueError(["Expected first layer id '%s' to match image id '%s'",
                          data[0], imagetagid])

        try:
            createdFiles = []
            createdDirs = []

            for layerid in data:
                layerdir = templatedir + "/" + layerid
                if not os.path.exists(layerdir):
                    os.makedirs(layerdir)
                    createdDirs.append(layerdir)

                jsonfile = layerdir + "/template.json"
                datafile = layerdir + "/template.tar.gz"

                if not os.path.exists(jsonfile) or not os.path.exists(datafile):
                    res = self._save_data(template,
                                          registryendpoint,
                                          "/v1/images/" + layerid + "/json",
                                          headers,
                                          jsonfile)
                    createdFiles.append(jsonfile)

                    datacsum = None
                    if layerid in checksums:
                        datacsum = checksums[layerid]

                    self._save_data(template,
                                    registryendpoint,
                                    "/v1/images/" + layerid + "/layer",
                                    headers,
                                    datafile, datacsum)
                    createdFiles.append(datafile)

            index = {
                "name": template.path,
            }

            indexfile = templatedir + "/" + imagetagid + "/index.json"
            print("Index file " + indexfile)
            with open(indexfile, "w") as f:
                 f.write(json.dumps(index))
        except Exception as e:
            traceback.print_exc()
            for f in createdFiles:
                try:
                    os.remove(f)
                except:
                    pass
            for d in createdDirs:
                try:
                    shutil.rmtree(d)
                except:
                    pass

    def _save_data(self, template, server, path, headers,
                   dest, checksum=None):
        try:
            res = self._get_url(template, server, path, headers)

            datalen = res.info().getheader("Content-Length")
            if datalen is not None:
                datalen = int(datalen)

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

    def _get_url(self, template, server, path, headers):
        if template.protocol is None:
            protocol = "https"
        else:
            protocol = template.protocol

        if server is None:
            if template.hostname is None:
                server = "index.docker.io"
            else:
                if template.port is not None:
                    server = "%s:%d" % (template.hostname, template.port)
                else:
                    server = template.hostname

        url = urlparse.urlunparse((protocol, server, path, None, None, None))
        debug("Fetching %s..." % url)

        req = urllib2.Request(url=url)
        if json:
            req.add_header("Accept", "application/json")
        for h in headers.keys():
            req.add_header(h, headers[h])

        #www Auth header starts
        if template.username and template.password:
            base64string = base64.encodestring(
                '%s:%s' % (template.username,
                           template.password)).replace('\n', '')
            req.add_header("Authorization", "Basic %s" % base64string)
        #www Auth header finish

        return urllib2.urlopen(req)

    def _get_json(self, template, server, path, headers):
        try:
            res = self._get_url(template, server, path, headers)
            data = json.loads(res.read())
            debug("OK\n")
            return (data, res)
        except Exception, e:
            debug("FAIL %s\n" % str(e))
            raise

    def create_template(self, template, templatedir, connect=None):
        if not self._was_downloaded(template, templatedir):
            self.download_template(template, templatedir)

        imagelist = self._get_image_list(template, templatedir)
        imagelist.reverse()

        parentImage = None
        for imagetagid in imagelist:
            templateImage = templatedir + "/" + imagetagid + "/template.qcow2"
            cmd = ["qemu-img","create","-f","qcow2"]
            if parentImage is not None:
                cmd.append("-o")
                cmd.append("backing_fmt=qcow2,backing_file=%s" % parentImage)
            cmd.append(templateImage)
            if parentImage is None:
                cmd.append("10G")
            subprocess.check_call(cmd)

            if parentImage is None:
                self.format_disk(templateImage, "qcow2", connect)

            path = templatedir + "/" + imagetagid + "/template."
            self.extract_tarball(path + "qcow2",
                                 "qcow2",
                                 path + "tar.gz",
                                 connect)
            parentImage = templateImage

    def _get_image_list(self, template, templatedir):
        imageparent = {}
        imagenames = {}
        imagedirs = os.listdir(templatedir)
        for imagetagid in imagedirs:
            indexfile = templatedir + "/" + imagetagid + "/index.json"
            if os.path.exists(indexfile):
                with open(indexfile,"r") as f:
                    index = json.load(f)
                imagenames[index["name"]] = imagetagid
            jsonfile = templatedir + "/" + imagetagid + "/template.json"
            if os.path.exists(jsonfile):
                with open(jsonfile,"r") as f:
                    data = json.load(f)
                parent = data.get("parent",None)
                if parent:
                    imageparent[imagetagid] = parent
        if not template.path in imagenames:
            raise ValueError(["Image %s does not exist locally" % template.path])
        imagetagid = imagenames[template.path]
        imagelist = []
        while imagetagid != None:
            imagelist.append(imagetagid)
            parent = imageparent.get(imagetagid,None)
            imagetagid = parent
        return imagelist

    def delete_template(self, template, templatedir):
        imageusage = {}
        imageparent = {}
        imagenames = {}
        imagedirs = os.listdir(templatedir)
        for imagetagid in imagedirs:
            indexfile = templatedir + "/" + imagetagid + "/index.json"
            if os.path.exists(indexfile):
                with open(indexfile,"r") as f:
                    index = json.load(f)
                imagenames[index["name"]] = imagetagid
            jsonfile = templatedir + "/" + imagetagid + "/template.json"
            if os.path.exists(jsonfile):
                with open(jsonfile,"r") as f:
                    data = json.load(f)

                parent = data.get("parent",None)
                if parent:
                    if parent not in imageusage:
                        imageusage[parent] = []
                    imageusage[parent].append(imagetagid)
                    imageparent[imagetagid] = parent


        if not template.path in imagenames:
            raise ValueError(["Image %s does not exist locally" % template.path])

        imagetagid = imagenames[template.path]
        while imagetagid != None:
            debug("Remove %s\n" % imagetagid)
            parent = imageparent.get(imagetagid,None)

            indexfile = templatedir + "/" + imagetagid + "/index.json"
            if os.path.exists(indexfile):
               os.remove(indexfile)
            jsonfile = templatedir + "/" + imagetagid + "/template.json"
            if os.path.exists(jsonfile):
                os.remove(jsonfile)
            datafile = templatedir + "/" + imagetagid + "/template.tar.gz"
            if os.path.exists(datafile):
                os.remove(datafile)
            imagedir = templatedir + "/" + imagetagid
            shutil.rmtree(imagedir)

            if parent:
                if len(imageusage[parent]) != 1:
                    debug("Parent %s is shared\n" % parent)
                    parent = None
            imagetagid = parent

    def _get_template_data(self, template, templatedir):
        imageList = self._get_image_list(template, templatedir)
        toplayer = imageList[0]
        diskfile = templatedir + "/" + toplayer + "/template.qcow2"
        configfile = templatedir + "/" + toplayer + "/template.json"
        return configfile, diskfile

    def get_disk(self, template, templatedir, imagedir, sandboxname):
        configfile, diskfile = self._get_template_data(template, templatedir)
        tempfile = imagedir + "/" + sandboxname + ".qcow2"
        if not os.path.exists(imagedir):
            os.makedirs(imagedir)
        cmd = ["qemu-img","create","-q","-f","qcow2"]
        cmd.append("-o")
        cmd.append("backing_fmt=qcow2,backing_file=%s" % diskfile)
        cmd.append(tempfile)
        subprocess.call(cmd)
        return tempfile

    def get_command(self, template, templatedir, userargs):
        configfile, diskfile = self._get_template_data(template, templatedir)
        configParser = DockerConfParser(configfile)
        cmd = configParser.getCommand()
        entrypoint = configParser.getEntrypoint()
        if entrypoint is None:
            entrypoint = []
        if cmd is None:
            cmd = []
        if userargs is not None and len(userargs) > 0:
            return entrypoint + userargs
        else:
            return entrypoint + cmd

    def get_env(self, template, templatedir):
        configfile, diskfile = self._get_template_data(template, templatedir)
        configParser = DockerConfParser(configfile)
        return configParser.getEnvs()

def debug(msg):
    sys.stderr.write(msg)
