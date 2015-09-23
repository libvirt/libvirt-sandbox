#
# -*- coding: utf-8 -*-
# Authors: Daniel P. Berrange <berrange@redhat.com>
#
# Copyright (C) 2015 Red Hat, Inc.
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

import urlparse
import importlib
import re

class Template(object):

    def __init__(self,
                 source, protocol,
                 hostname, port,
                 username, password,
                 path, params):
        """
        :param source: template source name
        :param protocol: network transport protocol or None
        :param hostname: registry hostname or None
        :param port: registry port or None
        :param username: username or None
        :param password: password or None
        :param path: template path identifier
        :param params: template parameters

        docker:///ubuntu

        docker+https://index.docker.io/ubuntu?tag=latest
        """

        self.source = source
        self.protocol = protocol
        self.hostname = hostname
        self.port = port
        self.username = username
        self.password = password
        self.path = path
        self.params = params
        if self.params is None:
            self.params = {}

    def get_source_impl(self):
        if self.source == "":
            raise Exception("Missing scheme in image URI")

        try:
            p = re.compile("\W")
            sourcename = "".join([i.capitalize() for i in p.split(self.source)])

            mod = importlib.import_module(
                "libvirt_sandbox.image.sources." +
                sourcename + "Source")
            classname = sourcename + "Source"
            classimpl = getattr(mod, classname)
            return classimpl()
        except Exception:
            raise Exception("Invalid source: '%s'" % self.source)

    def __repr__(self):
        if self.protocol is not None:
            scheme = self.source + "+" + self.protocol
        else:
            scheme = self.source
        if self.hostname:
            if self.port:
                netloc = "%s:%d" % (self.hostname, self.port)
            else:
                netloc = self.hostname

            if self.username:
                if self.password:
                    auth = self.username + ":" + self.password
                else:
                    auth = self.username
                netloc = auth + "@" + netloc
        else:
            netloc = None

        query = "&".join([key + "=" + self.params[key] for key in self.params.keys()])
        return urlparse.urlunparse((scheme, netloc, self.path, None, query, None))

    @classmethod
    def from_uri(klass, uri):
        o = urlparse.urlparse(uri)

        idx = o.scheme.find("+")
        if idx == -1:
            source = o.scheme
            protocol = None
        else:
            source = o.scheme[0:idx]
            protocol = o.scheme[idx + 1:]

        query = {}
        if o.query is not None and o.query != "":
            for param in o.query.split("&"):
                (key, val) = param.split("=")
                query[key] =  val
        return klass(source, protocol,
                     o.hostname, o.port,
                     o.username, o.password,
                     o.path, query)
