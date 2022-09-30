# THIS FILE WAS AUTO-GENERATED
#
#  $ lcitool manifest ci/manifest.yml
#
# https://gitlab.com/libvirt/libvirt-ci

FROM docker.io/library/ubuntu:22.04

RUN export DEBIAN_FRONTEND=noninteractive && \
    apt-get update && \
    apt-get install -y eatmydata && \
    eatmydata apt-get dist-upgrade -y && \
    eatmydata apt-get install --no-install-recommends -y \
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
                      zlib1g-dev && \
    eatmydata apt-get autoremove -y && \
    eatmydata apt-get autoclean -y && \
    sed -Ei 's,^# (en_US\.UTF-8 .*)$,\1,' /etc/locale.gen && \
    dpkg-reconfigure locales && \
    dpkg-query --showformat '${Package}_${Version}_${Architecture}\n' --show > /packages.txt && \
    mkdir -p /usr/libexec/ccache-wrappers && \
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/cc && \
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/gcc

ENV CCACHE_WRAPPERSDIR "/usr/libexec/ccache-wrappers"
ENV LANG "en_US.UTF-8"
ENV MAKE "/usr/bin/make"
ENV NINJA "/usr/bin/ninja"
ENV PYTHON "/usr/bin/python3"
