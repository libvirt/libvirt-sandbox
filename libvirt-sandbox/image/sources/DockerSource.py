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

    def create_template(self, templatename, templatedir, connect=None, format=None):
        if format is None:
            format = self.default_disk_format
        self._check_disk_format(format)
        imagelist = self._get_image_list(templatename,templatedir)
        imagelist.reverse()

        parentImage = None
        for imagetagid in imagelist:
            templateImage = templatedir + "/" + imagetagid + "/template." + format
            cmd = ["qemu-img","create","-f","qcow2"]
            if parentImage is not None:
                cmd.append("-o")
                cmd.append("backing_fmt=qcow2,backing_file=%s" % parentImage)
            cmd.append(templateImage)
            if parentImage is None:
                cmd.append("10G")
            subprocess.call(cmd)

            if parentImage is None:
                self._format_disk(templateImage,format,connect)

            self._extract_tarballs(templatedir + "/" + imagetagid + "/template.",format,connect)
            parentImage = templateImage


    def _check_disk_format(self,format):
        supportedFormats = ['qcow2']
        if not format in supportedFormats:
            raise ValueError(["Unsupported image format %s" % format])

    def _get_image_list(self,templatename,destdir):
        imageparent = {}
        imagenames = {}
        imagedirs = os.listdir(destdir)
        for imagetagid in imagedirs:
            indexfile = destdir + "/" + imagetagid + "/index.json"
            if os.path.exists(indexfile):
                with open(indexfile,"r") as f:
                    index = json.load(f)
                imagenames[index["name"]] = imagetagid
            jsonfile = destdir + "/" + imagetagid + "/template.json"
            if os.path.exists(jsonfile):
                with open(jsonfile,"r") as f:
                    template = json.load(f)
                parent = template.get("parent",None)
                if parent:
                    imageparent[imagetagid] = parent
        if not templatename in imagenames:
            raise ValueError(["Image %s does not exist locally" %templatename])
        imagetagid = imagenames[templatename]
        imagelist = []
        while imagetagid != None:
            imagelist.append(imagetagid)
            parent = imageparent.get(imagetagid,None)
            imagetagid = parent
        return imagelist

    def _format_disk(self,disk,format,connect):
        cmd = ['virt-sandbox']
        if connect is not None:
            cmd.append("-c")
            cmd.append(connect)
        cmd.append("-p")
        params = ['--disk=file:disk_image=%s,format=%s' %(disk,format),
                  '/sbin/mkfs.ext3',
                  '/dev/disk/by-tag/disk_image']
        cmd = cmd + params
        subprocess.call(cmd)

    def _extract_tarballs(self,directory,format,connect):
        tarfile = directory + "tar.gz"
        diskfile = directory + "qcow2"
        cmd = ['virt-sandbox']
        if connect is not None:
            cmd.append("-c")
            cmd.append(connect)
        cmd.append("-p")
        params = ['-m',
                  'host-image:/mnt=%s,format=%s' %(diskfile,format),
                  '--',
                  '/bin/tar',
                  'zxf',
                  '%s' %tarfile,
                  '-C',
                  '/mnt']
        cmd = cmd + params
        subprocess.call(cmd)

    def delete_template(self, templatename, templatedir):
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
                    template = json.load(f)

                parent = template.get("parent",None)
                if parent:
                    if parent not in imageusage:
                        imageusage[parent] = []
                    imageusage[parent].append(imagetagid)
                    imageparent[imagetagid] = parent


        if not templatename in imagenames:
            raise ValueError(["Image %s does not exist locally" %templatename])

        imagetagid = imagenames[templatename]
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

    def _get_template_data(self, templatename, templatedir):
        imageList = self._get_image_list(templatename, templatedir)
        toplayer = imageList[0]
        diskfile = templatedir + "/" + toplayer + "/template.qcow2"
        configfile = templatedir + "/" + toplayer + "/template.json"
        return configfile, diskfile

    def get_disk(self,templatename, templatedir, imagedir, sandboxname):
        configfile, diskfile = self._get_template_data(templatename, templatedir)
        tempfile = imagedir + "/" + sandboxname + ".qcow2"
        if not os.path.exists(imagedir):
            os.makedirs(imagedir)
        cmd = ["qemu-img","create","-q","-f","qcow2"]
        cmd.append("-o")
        cmd.append("backing_fmt=qcow2,backing_file=%s" % diskfile)
        cmd.append(tempfile)
        subprocess.call(cmd)
        return tempfile

    def get_command(self, templatename, templatedir, userargs):
        configfile, diskfile = self._get_template_data(templatename, templatedir)
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

    def get_env(self, templatename, templatedir):
        configfile, diskfile = self._get_template_data(templatename, templatedir)
        configParser = DockerConfParser(configfile)
        return configParser.getEnvs()

def debug(msg):
    sys.stderr.write(msg)
