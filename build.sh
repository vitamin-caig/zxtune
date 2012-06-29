#!/bin/sh

#builds one or more targets for specified architecture, platform and distribution
if [ $# -eq 0 ]; then
  echo "$0 platform-arch[-packaging[-distro]] targets"
  exit 0
fi

Mode=$1
if [ -z "${Mode}" ]; then
  echo "No mode specified"
  exit 1
fi;

Targets=$2
Platform=`echo ${Mode} | cut -d '-' -f 1`
Arch=`echo ${Mode} | cut -d '-' -f 2`
Packaging=`echo ${Mode} | cut -d '-' -f 3`
Distro=`echo ${Mode} | cut -d '-' -f 4`
Formats=txt
Languages=en

if [ -z "${Platform}" -o -z "${Arch}" ]; then
  echo "Invalid format"
  exit 1
fi

if [ -z "${Packaging}" ]; then
  Packaging="any"
  echo "No packaging specified. Using default '${Packaging}'"
fi

if [ -z "$2" ]; then
  Targets="xtractor zxtune123 zxtune-qt"
  echo "No targets specified. Using default '${Targets}'"
fi

# on some platforms qt and boost includes are not in standard path, so setup_* is required in any case
if [ "x${Distro}" != "xany" -a "x${Distro}" != "x" ]; then
  BOOST_VERSION=system
  QT_VERSION=system
fi
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

makecmd="make release=1 platform=${Platform} arch=${Arch} -C apps/"
if [ -z "${skip_clearing}" ]; then
  echo "Clearing"
  for target in ${Targets}
  do
    ${makecmd}${target} clean > /dev/null || exit 1
  done
fi

for target in ${Targets}
do
  echo "Building ${target} ${Platform}-${Arch}"
  time ${makecmd}${target} -j ${cpus} package packaging=${Packaging} distro=${Distro} ${options} cxx_flags="${cxx_flags}" ld_flags="${ld_flags}" && echo Done
done
