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

from abc import ABCMeta, abstractmethod
import subprocess

class Source():
    '''The Source class defines the base interface for
    all image provider source implementations. An image
    provide source is able to download templates from
    a repository, convert them to a host specific image
    format and report commands used to invoke them.'''

    __metaclass__ = ABCMeta
    def __init__(self):
        pass

    @abstractmethod
    def list_templates(self, templatedir):
        """
        :param templatedir: local directory path in which to store the template

        Get a list of all templates that are locally cached

        :returns: a list of libvirt_sandbox.template.Template objects
        """
        pass

    @abstractmethod
    def has_template(self, template, templatedir):
        """
        :param template: libvirt_sandbox.template.Template object
        :param templatedir: local directory path in which to store the template

        Check if a template has already been created.
        """
        pass

    @abstractmethod
    def create_template(self, template, templatedir,
                        connect=None):
        """
        :param template: libvirt_sandbox.template.Template object
        :param templatedir: local directory path in which to store the template
        :param connect: libvirt connection URI

        Create a set of local disk images populated with the content
        of a template. The images creation process will be isolated
        inside a sandbox using the requested libvirt connection URI.
        """
        pass

    @abstractmethod
    def delete_template(self, template, templatedir):
        """
        :param template: libvirt_sandbox.template.Template object
        :param templatedir: local directory path from which to delete template

        Delete all local files associated with the template
        """
        pass

    @abstractmethod
    def get_command(self, template, templatedir, userargs):
        """
        :param template: libvirt_sandbox.template.Template object
        :param templatedir: local directory path in which templates are stored
        :param userargs: user specified arguments to run

        Get the command line to invoke in the container. If userargs
        is specified, then this should override the default args in
        the image"""
        pass

    @abstractmethod
    def get_disk(self, template, templatedir, imagedir, sandboxname):
        """
        :param template: libvirt_sandbox.template.Template object
        :param templatedir: local directory path in which to find template
        :param imagedir: local directory in which to storage disk image

        Creates an instance private disk image, backed by the content
        of a template.
        """
        pass

    @abstractmethod
    def get_env(self, template, templatedir):
        """
        :param template: libvirt_sandbox.template.Template object
        :param templatedir: local directory path in which to find template

        Get the dict of environment variables to set
        """
        pass

    def post_run(self, template, templatedir, imagename):
        """
        :param template: libvirt_sandbox.template.Template object
        :param templatedir: local directory path in which to find template
        :param imagename: name of the image that just stopped running

        Hook called after the image has been stopped. By default is doesn't
        do anything, subclasses can override this to do some additional
        cleanup.
        """
        pass


    # Utility functions to share between the sources.

    def format_disk(self,disk,format,connect):
        cmd = ['virt-sandbox']
        if connect is not None:
            cmd.append("-c")
            cmd.append(connect)
        cmd.append("-p")
        params = ['--disk=file:disk_image=%s,format=%s' %(disk,format),
                  '/sbin/mkfs.ext3',
                  '/dev/disk/by-tag/disk_image']
        cmd = cmd + params
        subprocess.check_call(cmd)

    def extract_tarball(self, diskfile, format, tarfile, connect):
        cmd = ['virt-sandbox']
        if connect is not None:
            cmd.append("-c")
            cmd.append(connect)
        cmd.append("-p")
        compression = ""
        if tarfile.endswith(".gz"):
            compression = "z"
        params = ['-m',
                  'host-image:/mnt=%s,format=%s' % (diskfile, format),
                  '--',
                  '/bin/tar',
                  'xf%s' % compression,
                  '%s' % tarfile,
                  '-C',
                  '/mnt']
        cmd = cmd + params
        subprocess.check_call(cmd)
