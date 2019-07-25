#!/bin/bash

set -e

while [ "${1:0:2}" = "--" ]
do
  declare ${1:2}
  shift
done

echo "Pull ${branch:-master}"
git pull origin ${branch:-master}
cores=$(grep -c processor /proc/cpuinfo)
pvs-studio-analyzer trace -- make -j${cores} $*
all_excludes=",${excludes:-obj}"
#-a: 1-64 4-General Analysis 8-micro OPtimisations
pvs-studio-analyzer analyze -r $(pwd) -a 13 "${all_excludes//,/ -e }" -j${cores} -o analyze.log
format=${report_format:-fullhtml}
plog-converter -r $(pwd) ${plog_options:--a 'GA:1,2;64:1,2;OP:1,2'} -t ${format} -o report.${format} analyze.log
