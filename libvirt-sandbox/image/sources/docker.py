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
from abc import ABCMeta, abstractmethod
import copy
from libvirt_sandbox.image.template import Template

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

class DockerImage():

    def __init__(self, repo, name, tag=None):

        self.repo = repo
        self.name = name
        self.tag = tag

        if self.tag is None:
            self.tag = "latest"

        if self.repo is None:
            self.repo = "library"

    def __repr__(self):
        return "%s/%s,tag=%s" % (self.repo, self.name, self.tag)

    @classmethod
    def from_template(cls, template):
        bits = template.path[1:].split("/")
        if len(bits) == 1:
            return cls(repo=None,
                       name=bits[0],
                       tag=template.params.get("tag"))
        elif len(bits) == 2:
            return cls(repo=bits[0],
                       name=bits[1],
                       tag=template.params.get("tag"))
        else:
            raise Exception("Expected image name, or repo & image name for path, not '%s'",
                            template.path)


class DockerAuth():

    __metaclass__ = ABCMeta
    def __init__(self):
        pass

    @abstractmethod
    def prepare_req(self, req):
        pass

    @abstractmethod
    def process_res(self, res):
        pass

    @abstractmethod
    def process_err(self, err):
        return False


class DockerAuthNop(DockerAuth):

    def prepare_req(self, req):
        pass

    def process_res(self, res):
        pass

    def process_err(self, err):
        return False


class DockerAuthBasic(DockerAuth):

    def __init__(self, username, password):
        self.username = username
        self.password = password
        self.token = None

    def prepare_req(self, req):
        if self.username is not None:
            auth = base64.encodestring(
                '%s:%s' % (self.username, self.password)).replace('\n', '')

            req.add_header("Authorization", "Basic %s" % auth)

        req.add_header("X-Docker-Token", "true")

    def process_res(self, res):
        self.token = res.info().getheader('X-Docker-Token')

    def process_err(self, err):
        return False


class DockerAuthToken(DockerAuth):

    def __init__(self, token):
        self.token = token

    def prepare_req(self, req):
        req.add_header("Authorization", "Token %s" % self.token)

    def process_res(self, res):
        pass

    def process_err(self, err):
        return False


class DockerAuthBearer(DockerAuth):

    def __init__(self):
        self.token = None

    def prepare_req(self, req):
        if self.token is not None:
            req.add_header("Authorization", "Bearer %s" % self.token)

    def process_res(self, res):
        pass

    def process_err(self, err):
        method = err.headers.get("WWW-Authenticate", None)
        if method is None:
            return False

        if not method.startswith("Bearer "):
            return False

        challenge = method[7:]

        bits = challenge.split(",")
        attrs = {}
        for bit in bits:
            subbit = bit.split("=")
            attrs[subbit[0]] = subbit[1][1:-1]

        url = attrs["realm"]
        del attrs["realm"]
        if "error" in attrs:
            del attrs["error"]

        params = "&".join([
            "%s=%s" % (attr, attrs[attr])
            for attr in attrs.keys()
        ])
        if params != "":
            url = url + "?" + params

        req = urllib2.Request(url=url)
        req.add_header("Accept", "application/json")

        res = urllib2.urlopen(req)
        data = json.loads(res.read())
        self.token = data["token"]
        return True


class DockerRegistry():

    def __init__(self, uri_base):

        self.uri_base = list(urlparse.urlparse(uri_base))
        self.auth_handler = DockerAuthNop()

    def set_auth_handler(self, auth_handler):
        self.auth_handler = auth_handler

    def supports_v2(self):
        try:
            (data, res) = self.get_json("/v2/")
            ver = res.info().getheader("Docker-Distribution-Api-Version")
        except urllib2.HTTPError as e:
            ver = e.headers.get("Docker-Distribution-Api-Version", None)

        if ver is None:
            return False
        return ver.startswith("registry/2")

    def set_server(self, server):
        self.uri_base[1] = server

    @classmethod
    def from_template(cls, template):
        protocol = template.protocol
        hostname = template.hostname
        port = template.port

        if protocol is None:
            protocol = "https"
        if hostname is None:
            hostname = "index.docker.io"

        if port is None:
            server = hostname
        else:
            server = "%s:%s" % (hostname, port)

        url = urlparse.urlunparse((protocol, server, "", None, None, None))

        return cls(url)

    def get_url(self, path, headers=None):
        url_bits = copy.copy(self.uri_base)
        url_bits[2] = path
        url = urlparse.urlunparse(url_bits)
        debug("Fetching %s..." % url)

        req = urllib2.Request(url=url)

        if headers is not None:
            for h in headers.keys():
                req.add_header(h, headers[h])

        self.auth_handler.prepare_req(req)

        try:
            res = urllib2.urlopen(req)
            self.auth_handler.process_res(res)
            return res
        except urllib2.HTTPError as e:
            if e.code == 401:
                retry = self.auth_handler.process_err(e)
                if retry:
                    debug("Re-Fetching %s..." % url)
                    self.auth_handler.prepare_req(req)
                    res = urllib2.urlopen(req)
                    self.auth_handler.process_res(res)
                    return res
                else:
                    debug("Not re-fetching")
                    raise
            else:
                raise

    def save_data(self, path, dest, checksum=None):
        try:
            res = self.get_url(path)

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

    def get_json(self, path):
        try:
            headers = {}
            headers["Accept"] = "application/json"
            res = self.get_url(path, headers)
            data = json.loads(res.read())
            debug("OK\n")
            return (data, res)
        except Exception, e:
            debug("FAIL %s\n" % str(e))
            raise


class DockerSource(base.Source):

    def _check_cert_validate(self):
        major = sys.version_info.major
        SSL_WARNING = "SSL certificates couldn't be validated by default. You need to have 2.7.9/3.4.3 or higher"
        SSL_WARNING +="\nSee https://bugs.python.org/issue22417\n"
        py2_7_9_hexversion = 34015728
        py3_4_3_hexversion = 50594800
        if  (major == 2 and sys.hexversion < py2_7_9_hexversion) or (major == 3 and sys.hexversion < py3_4_3_hexversion):
            sys.stderr.write(SSL_WARNING)

    def _was_downloaded(self, image, templatedir):
        try:
            self._get_image_list(image, templatedir)
            return True
        except Exception:
            return False

    def list_templates(self, templatedir):
        indexes = []
        try:
            imagedirs = os.listdir(templatedir)
        except OSError:
            return []

        for imagetagid in imagedirs:
            indexfile = templatedir + "/" + imagetagid + "/index.json"
            if os.path.exists(indexfile):
                with open(indexfile,"r") as f:
                    index = json.load(f)
                    indexes.append(index)

        return [
            Template(source="docker",
                     protocol=None,
                     hostname=None,
                     port=None,
                     username=None,
                     password=None,
                     path="/%s/%s" % (index.get("repo", "library"), index["name"]),
                     params={
                         "tag": index.get("tag", "latest"),
                     }) for index in indexes]

    def has_template(self, template, templatedir):
        try:
            image = DockerImage.from_template(template)
            configfile, diskfile = self._get_template_data(image, templatedir)
            return os.path.exists(diskfile)
        except Exception:
            return False

    def download_template(self, image, template, templatedir):
        try:
            createdFiles = []
            createdDirs = []

            self._download_template_impl(image, template, templatedir, createdFiles, createdDirs)
        except Exception as e:
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
            raise

    def _download_template_impl(self, image, template, templatedir, createdFiles, createdDirs):
        self._check_cert_validate()

        registry = DockerRegistry.from_template(template)
        registry.set_auth_handler(DockerAuthBearer())
        if registry.supports_v2():
            self._download_template_impl_v2(registry, image, template, templatedir, createdFiles, createdDirs)
        else:
            self._download_template_impl_v1(registry, image, template, templatedir, createdFiles, createdDirs)

    def _download_template_impl_v1(self, registry, image, template, templatedir, createdFiles, createdDirs):
        basicauth = DockerAuthBasic(template.username, template.password)
        registry.set_auth_handler(basicauth)
        try:
            (data, res) = registry.get_json("/v1/repositories/%s/%s/images" % (
                                                image.repo, image.name,
                                            ))
        except urllib2.HTTPError, e:
            raise ValueError(["Image '%s' does not exist" % template])

        registryendpoint = res.info().getheader('X-Docker-Endpoints')

        if basicauth.token is not None:
            registry.set_auth_handler(DockerAuthToken(basicauth.token))
        else:
            registry.set_auth_handler(DockerAuthNop())

        (data, res) = registry.get_json("/v1/repositories/%s/%s/tags" %(
                                            image.repo, image.name
                                        ))

        if image.tag not in data:
            raise ValueError(["Tag '%s' does not exist for image '%s'" %
                              (image.tag, template)])
        imagetagid = data[image.tag]

        (data, res) = registry.get_json("/v1/images/" + imagetagid + "/ancestry")

        if data[0] != imagetagid:
            raise ValueError(["Expected first layer id '%s' to match image id '%s'",
                          data[0], imagetagid])

        for layerid in data:
            layerdir = templatedir + "/" + layerid
            if not os.path.exists(layerdir):
                os.makedirs(layerdir)
                createdDirs.append(layerdir)

            jsonfile = layerdir + "/template.json"
            datafile = layerdir + "/template.tar.gz"

            if not os.path.exists(jsonfile) or not os.path.exists(datafile):
                res = registry.save_data("/v1/images/" + layerid + "/json",
                                         jsonfile)
                createdFiles.append(jsonfile)

                registry.save_data("/v1/images/" + layerid + "/layer",
                                   datafile)
                createdFiles.append(datafile)

        index = {
            "repo": image.repo,
            "name": image.name,
            "tag": image.tag,
        }

        indexfile = templatedir + "/" + imagetagid + "/index.json"
        with open(indexfile, "w") as f:
            f.write(json.dumps(index))


    def _download_template_impl_v2(self, registry, image, template, templatedir, createdFiles, createdDirs):
        (manifest, res) = registry.get_json( "/v2/%s/%s/manifests/%s" % (
            image.repo, image.name, image.tag))

        layerChecksums = [
            layer["blobSum"] for layer in manifest["fsLayers"]
        ]
        layers = [
            json.loads(entry["v1Compatibility"]) for entry in manifest["history"]
        ]

        for i in range(len(layerChecksums)):
            layerChecksum = layerChecksums[i]
            config = layers[i]

            layerdir = templatedir + "/" + config["id"]
            if not os.path.exists(layerdir):
                os.makedirs(layerdir)
                createdDirs.append(layerdir)

            jsonfile = layerdir + "/template.json"
            datafile = layerdir + "/template.tar.gz"

            with open(jsonfile, "w") as fh:
                fh.write(json.dumps(config))

            registry.save_data("/v2/%s/%s/blobs/%s" % (
                                   image.repo, image.name, layerChecksum),
                               datafile, checksum=layerChecksum)


        index = {
            "repo": image.repo,
            "name": image.name,
            "tag": image.tag,
        }

        indexfile = templatedir + "/" + layers[0]["id"] + "/index.json"
        with open(indexfile, "w") as f:
            f.write(json.dumps(index))


    def create_template(self, template, templatedir, connect=None):
        image = DockerImage.from_template(template)

        if not self._was_downloaded(image, templatedir):
            self.download_template(image, template, templatedir)

        imagelist = self._get_image_list(image, templatedir)
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

    def _get_image_list(self, image, templatedir):
        imageparent = {}
        imagenames = {}
        imagedirs = []
        try:
            imagedirs = os.listdir(templatedir)
        except OSError:
            pass
        for imagetagid in imagedirs:
            indexfile = templatedir + "/" + imagetagid + "/index.json"
            if os.path.exists(indexfile):
                with open(indexfile,"r") as f:
                    index = json.load(f)
                thisimage = DockerImage(index.get("repo"),
                                        index["name"],
                                        index.get("tag"))
                imagenames[str(thisimage)] = imagetagid
            jsonfile = templatedir + "/" + imagetagid + "/template.json"
            if os.path.exists(jsonfile):
                with open(jsonfile,"r") as f:
                    data = json.load(f)
                parent = data.get("parent",None)
                if parent:
                    imageparent[imagetagid] = parent
        if str(image) not in imagenames:
            raise ValueError(["Image %s does not exist locally" % image])
        imagetagid = imagenames[str(image)]
        imagelist = []
        while imagetagid != None:
            imagelist.append(imagetagid)
            parent = imageparent.get(imagetagid,None)
            imagetagid = parent
        return imagelist

    def delete_template(self, template, templatedir):
        image = DockerImage.from_template(template)

        imageusage = {}
        imageparent = {}
        imagenames = {}
        imagedirs = []
        try:
            imagedirs = os.listdir(templatedir)
        except OSError:
            pass
        for imagetagid in imagedirs:
            indexfile = templatedir + "/" + imagetagid + "/index.json"
            if os.path.exists(indexfile):
                with open(indexfile,"r") as f:
                    index = json.load(f)
                thisimage = DockerImage(index.get("repo"),
                                        index["name"],
                                        index.get("tag"))
                imagenames[str(thisimage)] = imagetagid
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


        if str(image) not in imagenames:
            raise ValueError(["Image %s does not exist locally" % image])
        imagetagid = imagenames[str(image)]
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

    def _get_template_data(self, image, templatedir):
        imageList = self._get_image_list(image, templatedir)
        toplayer = imageList[0]
        diskfile = templatedir + "/" + toplayer + "/template.qcow2"
        configfile = templatedir + "/" + toplayer + "/template.json"
        return configfile, diskfile

    def get_disk(self, template, templatedir, imagedir, sandboxname):
        image = DockerImage.from_template(template)
        configfile, diskfile = self._get_template_data(image, templatedir)
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
        image = DockerImage.from_template(template)
        configfile, diskfile = self._get_template_data(image, templatedir)
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
        image = DockerImage.from_template(template)
        configfile, diskfile = self._get_template_data(image, templatedir)
        configParser = DockerConfParser(configfile)
        return configParser.getEnvs()

def debug(msg):
    sys.stderr.write(msg)
