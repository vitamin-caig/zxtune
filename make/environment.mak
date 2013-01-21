#android
# x86
android.x86.toolchain = /opt/x86-linux-android
android.x86.execprefix = $(android.x86.toolchain)/bin/i686-linux-android-
android.x86.boost.version = 1.49.0
# armeabi
android.armeabi.toolchain = /opt/armeabi-linux-android
android.armeabi.execprefix = $(android.armeabi.toolchain)/bin/arm-linux-androideabi-
android.armeabi.boost.version = 1.49.0
# armeabi-v7a
android.armeabi-v7a.toolchain = $(android.armeabi.toolchain)
android.armeabi-v7a.execprefix = $(android.armeabi.execprefix)
android.armeabi-v7a.boost.version = 1.49.0
# mips
android.mips.toolchain = /opt/mipsel-linux-android
android.mips.execprefix = $(android.mips.toolchain)/bin/mipsel-linux-android-
android.mips.boost.version = 1.49.0

#dingux
dingux.mipsel.toolchain = /opt/mipsel-linux-uclibc
dingux.mipsel.execprefix = $(dingux.mipsel.toolchain)/usr/bin/mipsel-linux-
dingux.mipsel.boost.version = 1.49.0
dingux.mipsel.qt.version = 4.7.1

#linux arm
linux.arm.toolchain = /opt/arm-linux
linux.arm.execprefix = $(linux.arm.toolchain)/bin/arm-none-linux-gnueabi-
linux.arm.boost.version = 1.49.0
linux.arm.boost.libs.model = -mt
linux.arm.qt.version = 4.8.1
linux.arm.crossroot = $(path_step)/../Build/linux-arm
