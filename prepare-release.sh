#!/bin/sh

set -e
set -v

# Make things clean.
test -n "$1" && RESULTS=$1 || RESULTS=results.log

test -f Makefile && make -k distclean || :

rm -rf build
mkdir build
cd build

../autogen.sh --prefix=$HOME/builder \
    --enable-gtk-doc

make
make install

# set -o pipefail is a bashism; this use of exec is the POSIX alternative
exec 3>&1
st=$(
  exec 4>&1 >&3
  { make check syntax-check 2>&1 3>&- 4>&-; echo $? >&4; } | tee "$RESULTS"
)
exec 3>&-
test "$st" = 0

rm -f *.tar.xz
make dist

if [ -f /usr/bin/rpmbuild ]; then
  rpmbuild --nodeps \
     --define "_sourcedir `pwd`" \
     -ba --clean libvirt-sandbox.spec
fi

exit 0
