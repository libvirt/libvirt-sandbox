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

from abc import ABCMeta, abstractmethod

class Source():
    '''The Source class defines the base interface for
    all image provider source implementations. An image
    provide source is able to download templates from
    a repository, convert them to a host specific image
    format and report commands used to invoke them.'''

    __metaclass__ = ABCMeta
    def __init__(self):
        pass
