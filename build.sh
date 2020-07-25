#!/bin/bash

find_make() {
  for mktool in gmake make
  do
    if ${mktool} -v > /dev/null 2>&1
    then
      echo ${mktool}
      return
    fi
  done
  echo "Failed to find make tool"
  exit 1
}

get_cpus() {
  if [ -e "/proc/cpuinfo" ]
  then
    grep processor /proc/cpuinfo | wc -l
  else
    sysctl -n hw.ncpu
  fi
}

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
Traits=(${Mode//-/ })
Platform=${Traits[0]}
Arch=${Traits[1]}
Packaging=${Traits[2]}
Distro=${Traits[3]}

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

Make=$(find_make)
Cpus=$(get_cpus)

makecmd="${Make} platform=${Platform} arch=${Arch} -C apps/"

for target in ${Targets}
do
  echo "Building ${target} ${Platform}-${Arch}"
  time ${makecmd}${target} -j ${Cpus} package packaging=${Packaging} distro=${Distro} && echo Done
done
