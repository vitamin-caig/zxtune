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
#-a: 1-64 4-General Analysis 8-micro OPtimisations
pvs-studio-analyzer analyze -r $(pwd) ${pvs_options:--a 13} -e obj -j${cores} -o analyze.log
format=${report_format:-fullhtml}
plog-converter -r $(pwd) ${plog_options:--a 'GA:1,2;64:1,2;OP:1,2'} -t ${format} -o report.${format} analyze.log
