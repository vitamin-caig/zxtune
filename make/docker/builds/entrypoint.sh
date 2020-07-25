#!/bin/bash

set -e

while [ "${1:0:2}" = "--" ]
do
  declare ${1:2}
  shift
done

if [ -n "${append_from_stdin:-}" ]
then
  echo "Append ${append_from_stdin} from stdin:"
  cat - >> "${append_from_stdin}"
fi

echo "Pull ${branch:-master}"
git pull origin ${branch:-master}
cores=$(grep -c processor /proc/cpuinfo)
make -j${jobs:-${cores}} $*
