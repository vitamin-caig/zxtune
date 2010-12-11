#!/bin/sh

rm -Rf zxtune*.xz
svn up || exit 1
makepkg -e || exit 1
package=`ls -1 *.xz`
dstdir=`echo $package | sed -nr 's/.*-r([0-9]+M?)-.-(.*)\.pkg\.tar\.xz/Builds\/Revision0\1_linux_\2/p'`
mkdir -p $dstdir
mv -f $package $dstdir
rm -Rf pkg
