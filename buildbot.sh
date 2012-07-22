#!/bin/sh

if [ ! -x ~/buildbot.config ]; then
  echo "No ~/buildbot.config found!"
  exit 1
fi

. ~/buildbot.config

echo "Updating"
svn up > /dev/null || (echo "Failed to update" && exit 1)

LastPlatform=
LastArch=
LastPackaging=
LastDistro=

for mode in ${build_modes}
do
  Platform=`echo ${mode} | cut -d '-' -f 1`
  Arch=`echo ${mode} | cut -d '-' -f 2`
  Packaging=`echo ${mode} | cut -d '-' -f 3`
  Distro=`echo ${mode} | cut -d '-' -f 4`
  skip_clearing=
  no_debuginfo=
  if [ "${LastPlatform}" = ${Platform} -a "${LastArch}" = "${Arch}" -a "${LastDistro}" = "${Distro}" ]; then
    skip_clearing=1
    no_debuginfo=1
  fi
  if [ "x${Platform}" = "xdingux" ]; then
    TOOLCHAIN_PATH=/opt/mipsel-linux-uclibc skip_clearing=${skip_clearing} no_debuginfo=${no_debuginfo} ./build.sh ${mode} "${build_targets}" || exit 1
  elif [ "x${Platform}" = "xlinux" -a "y${Arch}" = "yarm" ]; then
    TOOLCHAIN_PATH=/opt/arm-linux TOOLCHAIN_ROOT=`pwd`../Build/linux-arm skip_clearing=${skip_clearing} no_debuginfo=${no_debuginfo} ./build.sh ${mode} "${build_targets}" || exit 1
  else
    skip_clearing=${skip_clearing} no_debuginfo=${no_debuginfo} ./build.sh ${mode} "${build_targets}" || exit 1
  fi
  LastPlatform=${Platform}
  LastArch=${Arch}
  LastPackaging=${Packaging}
  LastDistro=${Distro}
done
