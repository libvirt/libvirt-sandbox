# THIS FILE WAS AUTO-GENERATED
#
#  $ lcitool manifest ci/manifest.yml
#
# https://gitlab.com/libvirt/libvirt-ci

FROM registry.fedoraproject.org/fedora:rawhide

RUN dnf update -y --nogpgcheck fedora-gpg-keys && \
    dnf install -y nosync && \
    echo -e '#!/bin/sh\n\
if test -d /usr/lib64\n\
then\n\
    export LD_PRELOAD=/usr/lib64/nosync/nosync.so\n\
else\n\
    export LD_PRELOAD=/usr/lib/nosync/nosync.so\n\
fi\n\
exec "$@"' > /usr/bin/nosync && \
    chmod +x /usr/bin/nosync && \
    nosync dnf distro-sync -y && \
    nosync dnf install -y \
               autoconf \
               automake \
               ca-certificates \
               ccache \
               cpp \
               gcc \
               gettext \
               gettext-devel \
               git \
               glib2-devel \
               glibc-langpack-en \
               glibc-static \
               gobject-introspection-devel \
               gtk-doc \
               libcap-ng-devel \
               libselinux-devel \
               libtirpc-devel \
               libtool \
               libvirt-devel \
               libvirt-gobject-devel \
               libxml2-devel \
               make \
               pkgconfig \
               rpcgen \
               rpm-build \
               xz \
               xz-devel \
               xz-static \
               zlib-devel \
               zlib-static && \
    nosync dnf autoremove -y && \
    nosync dnf clean all -y && \
    rpm -qa | sort > /packages.txt && \
    mkdir -p /usr/libexec/ccache-wrappers && \
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/cc && \
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/gcc

ENV CCACHE_WRAPPERSDIR "/usr/libexec/ccache-wrappers"
ENV LANG "en_US.UTF-8"
ENV MAKE "/usr/bin/make"
