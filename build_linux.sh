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
    which dpkg-deb >/dev/null 2>&1
    ;;
   archlinux)
    which makepkg >/dev/null 2>&1
    ;;
   redhat)
    which rpmbuild >/dev/null 2>&1
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
  Distros="any ubuntu archlinux redhat"
fi

unset skip_updating
for arc in ${Arches}
do
  unset skip_clearing
  unset no_debuginfo
  for dist in ${Distros}
  do
    if CanBuild ${dist} ${arc} ; then
      if skip_updating=${skip_updating} skip_clearing=${skip_clearing} no_debuginfo=${no_debuginfo} ./build.sh linux ${arc} ${dist} "${Targets}" ; then
        skip_clearing=1
        no_debuginfo=1
      fi
    else
      echo Build for ${dist}-${arc} is not supported
    fi
  done
done

