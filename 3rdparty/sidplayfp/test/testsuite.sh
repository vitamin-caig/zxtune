#!/bin/sh

dir=$(dirname $0)

logfile=$dir/testsuite.log
: > ${logfile}

passed=0
failed=0

while read line; do
    if [[ $line =~ ^# ]]; then continue; fi
    name=${line%% *}
    echo "Running test $name"
    $dir/test $line
    if [[ $? -ne 0 ]]; then
        failed=$((failed+1))
        echo "Failed test $name" >> ${logfile}
    else
        passed=$((passed+1))
    fi
done < $dir/testlist

total=$((passed+failed))
echo "Passed ${passed}/${total} ($((passed*100/total))%)"
echo "Failed ${failed}/${total}"

if [[ ${failed} -eq 0 ]]; then
    echo "Success!"
    exit 0
else
    echo "Failed tests logged in ${logfile}"
    exit 1
fi
