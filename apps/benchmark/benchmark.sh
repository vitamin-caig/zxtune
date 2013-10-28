#!/bin/sh
sed -e "/^$/q" /proc/cpuinfo > result.log
uname -a >> result.log
./benchmark >> result.log
