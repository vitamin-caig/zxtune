#!/bin/sh

./build.sh linux i686 any
skip_clearing=1 skip_updating=1 no_debuginfo=1 ./build.sh linux i686 ubuntu
skip_clearing=1 skip_updating=1 no_debuginfo=1 ./build.sh linux i686 archlinux
