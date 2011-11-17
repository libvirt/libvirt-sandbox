#!/bin/sh

set -e
set -v

# Make things clean.
test -f Makefile && make -k distclean || :

rm -rf build
mkdir build
cd build

../autogen.sh --prefix=$AUTOBUILD_INSTALL_ROOT \
    --enable-compile-warnings=error --enable-gtk-doc

make
make install

rm -f *.tar.gz
make dist

if [ -n "$AUTOBUILD_COUNTER" ]; then
  EXTRA_RELEASE=".auto$AUTOBUILD_COUNTER"
else
  NOW=`date +"%s"`
  EXTRA_RELEASE=".$USER$NOW"
fi

if [ -f /usr/bin/rpmbuild ]; then
  rpmbuild --nodeps \
     --define "extra_release $EXTRA_RELEASE" \
     --define "_sourcedir `pwd`" \
     -ba --clean libvirt-sandbox.spec
fi

exit 0
if [ -x /usr/bin/i686-pc-mingw32-gcc ]; then
  make distclean

  PKG_CONFIG_PATH="$AUTOBUILD_INSTALL_ROOT/i686-pc-mingw32/sys-root/mingw/lib/pkgconfig" \
  CC="i686-pc-mingw32-gcc" \
  ../configure \
    --build=$(uname -m)-pc-linux \
    --host=i686-pc-mingw32 \
    --prefix="$AUTOBUILD_INSTALL_ROOT/i686-pc-mingw32/sys-root/mingw" \
    --without-python

  make
  make install

  #set -o pipefail
  #make check 2>&1 | tee "$RESULTS"

  if [ -f /usr/bin/rpmbuild ]; then
    rpmbuild --nodeps \
       --define "extra_release $EXTRA_RELEASE" \
       --define "_sourcedir `pwd`" \
       -ba --clean mingw32-libvirt-sandbox.spec
  fi
fi
