#!/usr/bin/python
# -*- coding: utf-8 -*-
#
# Copyright (C) 2015 Universitat Politècnica de Catalunya.
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

from Source import Source
import urllib2
import sys
import json
import traceback
import os
import subprocess
import shutil

class DockerSource(Source):

    www_auth_username = None
    www_auth_password = None

    def __init__(self):
        self.default_index_server = "index.docker.io"

    def _check_cert_validate(self):
        major = sys.version_info.major
        SSL_WARNING = "SSL certificates couldn't be validated by default. You need to have 2.7.9/3.4.3 or higher"
        SSL_WARNING +="\nSee https://bugs.python.org/issue22417\n"
        py2_7_9_hexversion = 34015728
        py3_4_3_hexversion = 50594800
        if  (major == 2 and sys.hexversion < py2_7_9_hexversion) or (major == 3 and sys.hexversion < py3_4_3_hexversion):
            sys.stderr.write(SSL_WARNING)

    def download_template(self, templatename, templatedir,
                          registry=None, username=None, password=None):
        if registry is None:
            registry = self.default_index_server

        if username is not None:
            self.www_auth_username = username
            self.www_auth_password = password

        self._check_cert_validate()
        tag = "latest"
        offset = templatename.find(':')
        if offset != -1:
            tag = templatename[offset + 1:]
            templatename = templatename[0:offset]
        try:
            (data, res) = self._get_json(registry, "/v1/repositories/" + templatename + "/images",
                               {"X-Docker-Token": "true"})
        except urllib2.HTTPError, e:
            raise ValueError(["Image '%s' does not exist" % templatename])

        registryendpoint = res.info().getheader('X-Docker-Endpoints')
        token = res.info().getheader('X-Docker-Token')
        checksums = {}
        for layer in data:
            pass
        (data, res) = self._get_json(registryendpoint, "/v1/repositories/" + templatename + "/tags",
                           { "Authorization": "Token " + token })

        cookie = res.info().getheader('Set-Cookie')

        if not tag in data:
            raise ValueError(["Tag '%s' does not exist for image '%s'" % (tag, templatename)])
        imagetagid = data[tag]

        (data, res) = self._get_json(registryendpoint, "/v1/images/" + imagetagid + "/ancestry",
                               { "Authorization": "Token "+token })

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
                    res = self._save_data(registryendpoint, "/v1/images/" + layerid + "/json",
                                { "Authorization": "Token " + token }, jsonfile)
                    createdFiles.append(jsonfile)

                    layersize = int(res.info().getheader("Content-Length"))

                    datacsum = None
                    if layerid in checksums:
                        datacsum = checksums[layerid]

                    self._save_data(registryendpoint, "/v1/images/" + layerid + "/layer",
                          { "Authorization": "Token "+token }, datafile, datacsum, layersize)
                    createdFiles.append(datafile)

            index = {
                "name": templatename,
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
    def _save_data(self,server, path, headers, dest, checksum=None, datalen=None):
        try:
            res = self._get_url(server, path, headers)

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

    def _get_url(self,server, path, headers):
        url = "https://" + server + path
        debug("Fetching %s..." % url)

        req = urllib2.Request(url=url)
        if json:
            req.add_header("Accept", "application/json")
        for h in headers.keys():
            req.add_header(h, headers[h])

        #www Auth header starts
        if self.www_auth_username is not None:
            base64string = base64.encodestring('%s:%s' % (self.www_auth_username, self.www_auth_password)).replace('\n', '')
            req.add_header("Authorization", "Basic %s" % base64string)
        #www Auth header finish

        return urllib2.urlopen(req)

    def _get_json(self,server, path, headers):
        try:
            res = self._get_url(server, path, headers)
            data = json.loads(res.read())
            debug("OK\n")
            return (data, res)
        except Exception, e:
            debug("FAIL %s\n" % str(e))
            raise

def debug(msg):
    sys.stderr.write(msg)
