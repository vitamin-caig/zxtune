#!/bin/sh

CanBuild()
{
  local dist=$1;
  local arc=$2
  case ${dist} in
   any)
    test -e ../Build/boost-linux-${arc} && test -e ../Build/qt-linux-${arc}
    ;;
   ubuntu)
    which dpkg-buildpackage >/dev/null 2>&1
    ;;
   ubuntu:any)
    CanBuild ubuntu ${arc} && CanBuild any ${arc}
    ;;
   archlinux)
    which makepkg >/dev/null 2>&1
    ;;
   archlinux:any)
    CanBuild archlinux ${arc} && CanBuild any ${arc}
    ;;
   redhat)
    which rpmbuild >/dev/null 2>&1
    ;;
   redhat:any)
    CanBuild redhat ${arc} && CanBuild any ${arc}
    ;;
   dingux)
    test -e /opt/mipsel-linux-uclibc && test -e ../Build/boost-dingux-${arc} && test -e ../Build/qt-dingux-${arc}
    ;;
   *)
    false
    ;;
  esac
}

Arches=$1
Distros=$2
Targets=$3

if [ -z "${Arches}" ]; then
  echo "No architectures specified"
  exit 1
fi

if [ -z "${Distros}" ]; then
  Distros="any ubuntu archlinux redhat dingux"
fi

unset skip_updating
for arc in ${Arches}
do
  for dist in ${Distros}
  do
    if [ "x${dist}" = "xdingux" ]; then
      platform="dingux"
      arc=mipsel
      real_dist=any
    else
      platform="linux"
      real_dist=${dist}
    fi
    if CanBuild ${dist} ${arc} ; then
      if skip_updating=${skip_updating} TOOLCHAIN_PATH=/opt/mipsel-linux-uclibc ./build.sh ${platform} ${arc} ${real_dist} "${Targets}" ; then
        skip_updating=1
      else
        echo Failed
      fi
    else
      echo Build for ${dist}-${arc} is not supported
    fi
  done
done

