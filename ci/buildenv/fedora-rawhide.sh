# THIS FILE WAS AUTO-GENERATED
#
#  $ lcitool manifest ci/manifest.yml
#
# https://gitlab.com/libvirt/libvirt-ci

function install_buildenv() {
    dnf update -y --nogpgcheck fedora-gpg-keys
    dnf distro-sync -y
    dnf install -y \
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
        zlib-static
    rpm -qa | sort > /packages.txt
    mkdir -p /usr/libexec/ccache-wrappers
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/cc
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/gcc
}

export CCACHE_WRAPPERSDIR="/usr/libexec/ccache-wrappers"
export LANG="en_US.UTF-8"
export MAKE="/usr/bin/make"
