#!/bin/sh
mode=${1-release}
LD_LIBRARY_PATH=../../bin/${mode} valgrind -v --log-file=memory.log --leak-check=full --show-reachable=yes --track-origins=yes ../../bin/${mode}/error_test
