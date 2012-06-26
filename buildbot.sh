#!/bin/sh

if [ ! -x buildbot.config ]; then
  echo "No buildbot.config found!"
  exit 1
fi

. ./buildbot.config

./build_linux.sh "${build_architectures}" "${build_distribs}" "${build_targets}"
