#!/bin/bash

if [ ! -x ~/buildbot.config ]; then
  echo "No ~/buildbot.config found!"
  exit 1
fi

. ~/buildbot.config

LastPlatform=
LastArch=
LastDistro=

for mode in ${build_modes}
do
  Traits=(${mode//-/ })
  Platform=${Traits[0]}
  Arch=${Traits[1]}
  Packaging=${Traits[2]}
  Distro=${Traits[3]}
  no_debuginfo=
  if [ "${LastPlatform}" = ${Platform} -a "${LastArch}" = "${Arch}" -a "${LastDistro}" = "${Distro}" ]; then
    no_debuginfo=1
  fi
  no_debuginfo=${no_debuginfo} ./build.sh ${mode} "${build_targets}" || exit 1
  LastPlatform=${Platform}
  LastArch=${Arch}
  LastDistro=${Distro}
done
