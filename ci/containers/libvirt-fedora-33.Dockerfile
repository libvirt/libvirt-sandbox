FROM registry.fedoraproject.org/fedora:33

RUN dnf update -y && \
    dnf install -y \
        autoconf \
        automake \
        ca-certificates \
        ccache \
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
        xz-devel \
        xz-static \
        zlib-devel \
        zlib-static && \
    dnf autoremove -y && \
    dnf clean all -y && \
    mkdir -p /usr/libexec/ccache-wrappers && \
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/cc && \
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/$(basename /usr/bin/gcc)

ENV LANG "en_US.UTF-8"
ENV MAKE "/usr/bin/make"
ENV CCACHE_WRAPPERSDIR "/usr/libexec/ccache-wrappers"
