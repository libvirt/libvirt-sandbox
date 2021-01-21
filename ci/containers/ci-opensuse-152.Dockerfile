# THIS FILE WAS AUTO-GENERATED
#
#  $ lcitool dockerfile opensuse-152 libvirt+dist,libvirt-glib+dist,libvirt-sandbox
#
# https://gitlab.com/libvirt/libvirt-ci/-/commit/740f5254f607de914a92d664196d045149edb45a
FROM registry.opensuse.org/opensuse/leap:15.2

RUN zypper update -y && \
    zypper install -y \
           autoconf \
           automake \
           ca-certificates \
           ccache \
           gcc \
           gettext-devel \
           gettext-runtime \
           git \
           glib2-devel \
           glibc-locale \
           glibc-static \
           gobject-introspection-devel \
           gtk-doc \
           libcap-ng-devel \
           libselinux-devel \
           libtirpc-devel \
           libtool \
           libvirt-devel \
           libvirt-glib-devel \
           libxml2-devel \
           make \
           pkgconfig \
           rpcgen \
           rpm-build \
           xz-devel \
           xz-static-devel \
           zlib-devel \
           zlib-devel-static && \
    zypper clean --all && \
    rpm -qa | sort > /packages.txt && \
    mkdir -p /usr/libexec/ccache-wrappers && \
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/cc && \
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/$(basename /usr/bin/gcc)

ENV LANG "en_US.UTF-8"
ENV MAKE "/usr/bin/make"
ENV CCACHE_WRAPPERSDIR "/usr/libexec/ccache-wrappers"
