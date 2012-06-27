#!/bin/sh

#builds one or more targets for specified architecture, platform and distribution

Platform=$1
Arch=$2
Distro=$3
Targets=$4
Formats=txt
Languages=en

if [ -z "${Targets}" ]; then
  Targets="xtractor zxtune123 zxtune-qt"
fi

if [ -z "${skip_updating}" ]; then
# get current build and vesion
echo "Updating"
svn up > /dev/null || (echo "Failed to update" && exit 1)
fi

DistroType=`echo ${Distro} | cut -d ':' -f 2`
Distro=`echo ${Distro} | cut -d ':' -f 1`
if [ "x${DistroType}" != "xany" ]; then
  BOOST_VERSION=system
  QT_VERSION=system
fi
# on some platforms qt and boost includes are not in standard path, so setup_* is required in any case
. make/platforms/setup_qt.sh || exit 1
. make/platforms/setup_boost.sh || exit 1
# checking for textator or assume that texts are correct
textator --version > /dev/null 2>&1 || export NO_TEXTATOR=1

cpus=`grep processor /proc/cpuinfo | wc -l`

# adding additional platform properties if required
case ${Arch} in
    ppc | ppc64 | powerpc)
      test `grep cpu /proc/cpuinfo | uniq | cut -f 3 -d " "` = "altivec" && cxx_flags="-mabi=altivec -maltivec"
      test `grep cpu /proc/cpuinfo | uniq | cut -f 2 -d " " | sed -e 's/,//g'` = "PPC970MP" && cxx_flags="${cxx_flags} -mtune=970 -mcpu=970"
    ;;
    x86_64 | amd64)
      cxx_flags="-m64 -march=x86-64 -mtune=generic -mmmx"
      ld_flags="-m64"
    ;;
    i386 | i486 | i586 | i686)
      cxx_flags="-m32 -march=${Arch} -mtune=generic -mmmx"
      ld_flags="-m32"
    ;;
    *)
    ;;
esac

makecmd="make platform=${Platform} release=1 arch=${Arch} distro=${Distro} -C apps/"
if [ -z "${skip_clearing}" ]; then
  echo "Clearing"
  for target in ${Targets}
  do
    ${makecmd}${target} clean > /dev/null || exit 1
  done
fi

for target in ${Targets}
do
  echo "Building ${target} ${Platform}-${Distro}-${Arch}"
  time ${makecmd}${target} -j ${cpus} package ${options} cxx_flags="${cxx_flags}" ld_flags="${ld_flags}" && echo Done
done
