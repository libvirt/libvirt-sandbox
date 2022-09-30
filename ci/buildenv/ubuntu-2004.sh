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
            liblzma-dev \
            libselinux1-dev \
            libtirpc-dev \
            libtool \
            libtool-bin \
            libvirt-dev \
            libvirt-glib-1.0-dev \
            libxml2-dev \
            locales \
            make \
            pkgconf \
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
