#!/bin/sh

Binaries=$1
Platform=$2
Mode=release
Arch=$3
Formats=txt
Languages=en

# checking for textator or assume that texts are correct
textator --version > /dev/null 2>&1 || touch text/*.cpp text/*.h

# get current build and vesion
echo "Updating"
svn up > /dev/null || (echo "Failed to update"; exit 1)

echo "Clearing"
rm -Rf lib/${Platform}/${Mode} obj/${Platform}/${Mode} || exit 1;

# adding additional platform properties if required
case ${Arch} in
    ppc | ppc64 | powerpc)
	test `grep cpu /proc/cpuinfo | uniq | cut -f 3 -d " "` = "altivec" && cxx_flags="-mabi=altivec -maltivec"
	test `grep cpu /proc/cpuinfo | uniq | cut -f 2 -d " " | sed -e 's/,//g'` = "PPC970MP" && cxx_flags="${cxx_flags} -mtune=970 -mcpu=970"
    ;;
    *)
    ;;
esac

for Binary in ${Binaries}
do
echo "Building ${Binary} with mode=${Mode} platform=${Platform} arch=${Arch}"
time make -j `grep processor /proc/cpuinfo | wc -l` package mode=${Mode} platform=${Platform} arch=${Arch} cxx_flags="${cxx_flags}" -C apps/${Binary} || exit 1;
done
echo Done
