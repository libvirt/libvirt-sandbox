FROM registry.opensuse.org/opensuse/leap:15.1

RUN zypper update -y && \
    zypper install -y \
           autoconf \
           automake \
           ca-certificates \
           ccache \
           gcc \
           gettext \
           gettext-devel \
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
    mkdir -p /usr/libexec/ccache-wrappers && \
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/cc && \
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/$(basename /usr/bin/gcc)

ENV LANG "en_US.UTF-8"
ENV MAKE "/usr/bin/make"
ENV CCACHE_WRAPPERSDIR "/usr/libexec/ccache-wrappers"
