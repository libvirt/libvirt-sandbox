# THIS FILE WAS AUTO-GENERATED
#
#  $ lcitool manifest ci/manifest.yml
#
# https://gitlab.com/libvirt/libvirt-ci

function install_buildenv() {
    export DEBIAN_FRONTEND=noninteractive
    apt-get update
    apt-get dist-upgrade -y
    apt-get install --no-install-recommends -y \
            autoconf \
            automake \
            autopoint \
            ca-certificates \
            ccache \
            cpp \
            gcc \
            gettext \
            git \
            gtk-doc-tools \
            libc-dev-bin \
            libc6-dev \
            libcap-ng-dev \
            libgirepository1.0-dev \
            libglib2.0-dev \
            libglib2.0-dev-bin \
            libgnutls28-dev \
            liblzma-dev \
            libnl-3-dev \
            libnl-route-3-dev \
            libselinux1-dev \
            libtirpc-dev \
            libtool \
            libtool-bin \
            libxml2-dev \
            libxml2-utils \
            locales \
            make \
            meson \
            ninja-build \
            perl-base \
            pkgconf \
            python3 \
            python3-docutils \
            valac \
            xsltproc \
            xz-utils \
            zlib1g-dev
    sed -Ei 's,^# (en_US\.UTF-8 .*)$,\1,' /etc/locale.gen
    dpkg-reconfigure locales
    dpkg-query --showformat '${Package}_${Version}_${Architecture}\n' --show > /packages.txt
    mkdir -p /usr/libexec/ccache-wrappers
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/cc
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/gcc
}

export CCACHE_WRAPPERSDIR="/usr/libexec/ccache-wrappers"
export LANG="en_US.UTF-8"
export MAKE="/usr/bin/make"
export NINJA="/usr/bin/ninja"
export PYTHON="/usr/bin/python3"
