#!/usr/bin/python
#
# Copyright (C) 2015 SUSE LLC
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
# Author: Cedric Bosdonnat <cbosdonnat@suse.com>
#

import os
import os.path
import subprocess

from . import base
from libvirt_sandbox.image.template import Template


class VirtBuilderSource(base.Source):

    def _get_template_name(self, template):
        # We shouldn't have '/' in the names, but let's make sure
        # nobody can try to alter the folders structure later.
        return template.path[1:].replace('/', '_')

    def has_template(self, template, templatedir):
        imagepath = "%s/%s.qcow2" % (templatedir, template.path)
        return os.path.exists(imagepath)

    def create_template(self, template, templatedir, connect=None):
        if not os.path.exists(templatedir):
            os.makedirs(templatedir)

        # Get the image using virt-builder
        templatename = self._get_template_name(template)
        imagepath_original = "%s/%s-original.qcow2" % (templatedir, templatename)
        imagepath = "%s/%s.qcow2" % (templatedir, templatename)
        cmd = ["virt-builder", templatename, "--no-network",
               "-o", imagepath_original, "--format", "qcow2"]
        subprocess.check_call(cmd)

        try:
            # We need to convert this image into a single partition one.
            tarfile = "%s/%s.tar" % (templatedir, templatename)
            cmd = ["virt-tar-out", "-a", imagepath_original, "/", tarfile]
            subprocess.check_call(cmd)

            cmd = ["qemu-img", "create", "-q", "-f", "qcow2", imagepath, "10G"]
            subprocess.check_call(cmd)

            self.format_disk(imagepath, "qcow2", connect)
            self.extract_tarball(imagepath, "qcow2", tarfile, connect)

        finally:
            os.unlink(imagepath_original)
            os.unlink(tarfile)

    def list_templates(self, templatedir):
        files = []
        try:
            imagefiles = os.listdir(templatedir)
        except OSError:
            return []

        for filename in imagefiles:
            if not filename.endswith(".qcow2"):
                continue
            files.append(filename[0:-6])

        return [
            Template(source="virt-builder",
                     protocol=None,
                     hostname=None,
                     port=None,
                     username=None,
                     password=None,
                     path="/%s" % filename,
                     params={}) for filename in files]

    def delete_template(self, template, templatedir):
        os.unlink("%s/%s.qcow2" % (templatedir, self._get_template_name(template)))

    def get_command(self, template, templatedir, userargs):
        return userargs

    def get_disk(self,template, templatedir, imagedir, sandboxname):
        diskfile = "%s/%s.qcow2" % (templatedir, self._get_template_name(template))
        tempfile = imagedir + "/" + sandboxname + ".qcow2"
        if not os.path.exists(imagedir):
            os.makedirs(imagedir)
        cmd = ["qemu-img", "create", "-q",
               "-f", "qcow2",
               "-o", "backing_fmt=qcow2,backing_file=%s" % diskfile,
               tempfile]
        subprocess.check_call(cmd)
        return tempfile

    def get_env(self,template, templatedir):
        return []
