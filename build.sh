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

cpus=`grep processor /proc/cpuinfo | wc -l`

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
  time ${makecmd}${target} -j ${cpus} package packaging=${Packaging} distro=${Distro} && echo Done
done
