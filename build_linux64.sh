#!/bin/sh

./build.sh linux x86_64 any
skip_clearing=1 skip_updating=1 no_debuginfo=1 ./build.sh linux x86_64 ubuntu
skip_clearing=1 skip_updating=1 no_debuginfo=1 ./build.sh linux x86_64 archlinux
