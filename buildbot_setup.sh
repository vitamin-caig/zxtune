#!/bin/sh

test "x$1" != "x--recreate" && test -e ~/buildbot.config && . ~/buildbot.config

if [ -z "${build_targets}" ]; then
  echo "Setting up default targets"
  build_targets="zxtune123 zxtune-qt"
fi

if [ -z "${build_architectures}" ]; then
  echo "Detecting available architectures"
  build_architectures=`uname -m`
fi

if [ -z "${build_distribs}" ]; then
  echo "Detecting available distribs"
  test -e ../Build/boost-linux-`uname -m` && test -e ../Build/qt-linux-`uname -m` && build_distribs="any"
  which dpkg-buildpackage >/dev/null 2>&1 && build_distribs="${build_distribs+${build_distribs} }ubuntu"
  which makepkg >/dev/null 2>&1 && build_distribs="${build_distribs+${build_distribs} }archlinux"
  which rpmbuild >/dev/null 2>&1 && build_distribs="${build_distribs+${build_distribs} }redhat"
  test -e /opt/mipsel-linux-uclibc && test -e ../Build/boost-dingux-mipsel && test -e ../Build/qt-dingux-mipsel && build_distribs="${build_distribs+${build_distribs} }dingux"
fi

read -p "Targets [${build_targets}]: " build_targets_new
read -p "Architectures [${build_architectures}]: " build_architectures_new
read -p "Distros [${build_distribs}]: " build_distribs_new

test -n "${build_targets_new}" && build_targets=${build_targets_new}
test -n "${build_architectures_new}" && build_architectures=${build_architectures_new}
test -n "${build_distribs_new}" && build_distribs=${build_distribs_new}

printf "\
#!/bin/sh\n\
build_targets=\"%s\"\n\
build_architectures=\"%s\"\n\
build_distribs=\"%s\"\n\
" "${build_targets}" "${build_architectures}" "${build_distribs}" > ~/buildbot.config
chmod +x ~/buildbot.config
