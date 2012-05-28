#!/bin/sh

Arch=$1

if [ -z "${Arch}" ]; then
  echo "No architecture specified"
  exit 1
fi

./build.sh linux ${Arch} any
which dpkg-deb >/dev/null 2>&1 && skip_clearing=1 skip_updating=1 no_debuginfo=1 ./build.sh linux ${Arch} ubuntu || echo "No dpkg-deb found"
which makepkg >/dev/null 2>&1 && skip_clearing=1 skip_updating=1 no_debuginfo=1 ./build.sh linux ${Arch} archlinux || echo "No makepkg found"
which rpmbuild >/dev/null 2>&1 && skip_clearing=1 skip_updating=1 no_debuginfo=1 ./build.sh linux ${Arch} redhat || echo "No rpmbuild found"
