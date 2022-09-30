# THIS FILE WAS AUTO-GENERATED
#
#  $ lcitool manifest ci/manifest.yml
#
# https://gitlab.com/libvirt/libvirt-ci

function install_buildenv() {
    zypper dist-upgrade -y
    zypper install -y \
           autoconf \
           automake \
           ca-certificates \
           ccache \
           cpp \
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
           xz \
           xz-devel \
           xz-static-devel \
           zlib-devel \
           zlib-devel-static
    rpm -qa | sort > /packages.txt
    mkdir -p /usr/libexec/ccache-wrappers
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/cc
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/gcc
}

export CCACHE_WRAPPERSDIR="/usr/libexec/ccache-wrappers"
export LANG="en_US.UTF-8"
export MAKE="/usr/bin/make"
