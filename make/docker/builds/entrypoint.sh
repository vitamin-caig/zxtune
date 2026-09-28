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

echo 'Environment'
env
vars=$(git rev-parse --show-toplevel)/variables.mak
if [ -e "${vars}" ]
then
  echo 'Store variables'
  # ./gradlew runs under /bin/sh, whose shell discards environment entries whose
  # names are not valid identifiers, so these never reach the make build that
  # Gradle runs; record them where the makefiles read them from instead
  env | grep -E '^[A-Za-z_][A-Za-z0-9_]*(\.[A-Za-z0-9_]+)+=' | tee -a "${vars}"
fi

cores=$(grep -c processor /proc/cpuinfo)
make -j${jobs:-${cores}} $*
