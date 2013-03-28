#apply default values
toolchains.root ?= /opt
prebuilt.dir ?= $(path_step)/../Build

#android
android.cxx.flags = -no-canonical-prefixes -funwind-tables -fstack-protector -fomit-frame-pointer -finline-limit=300 -Wa,--noexecstack
android.ld.flags = -no-canonical-prefixes -Wl,-soname,$(notdir $@) -Wl,--no-undefined -Wl,-z,noexecstack -Wl,-z,relro -Wl,-z,now
#assume that all the platforms are little-endian
#this required to use boost which doesn't know anything about __armel__ or __mipsel__
android.definitions = ANDROID __ANDROID__ __LITTLE_ENDIAN__
# x86
android.x86.toolchain = $(toolchains.root)/x86-linux-android
android.x86.execprefix = $(android.x86.toolchain)/bin/i686-linux-android-
android.x86.boost.version = 1.49.0
android.x86.ld.flags = -Wl,--icf=safe
# armeabi
android.armeabi.toolchain = $(toolchains.root)/armeabi-linux-android
android.armeabi.execprefix = $(android.armeabi.toolchain)/bin/arm-linux-androideabi-
android.armeabi.boost.version = 1.49.0
android.armeabi.cxx.flags = -march=armv5te -mtune=xscale -msoft-float -mthumb
android.armeabi.definitions = __ARM_ARCH_5__ __ARM_ARCH_5T__ __ARM_ARCH_5E__ __ARM_ARCH_5TE__
android.armeabi.ld.flags = $(android.x86.ld.flags)
# armeabi-v7a
android.armeabi-v7a.toolchain = $(android.armeabi.toolchain)
android.armeabi-v7a.execprefix = $(android.armeabi.execprefix)
android.armeabi-v7a.boost.version = 1.49.0
android.armeabi-v7a.cxx.flags = -march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16 -mthumb
android.armeabi-v7a.ld.flags = -march=armv7-a -Wl,--fix-cortex-a8
android.armeabi-v7a.definitions = $(android.armeabi.definitions)
android.armeabi-v7a.ld.flags = $(android.x86.ld.flags)
# mips
android.mips.toolchain = $(toolchains.root)/mipsel-linux-android
android.mips.execprefix = $(android.mips.toolchain)/bin/mipsel-linux-android-
android.mips.boost.version = 1.49.0
android.mips.cxx.flags = -fno-inline-functions-called-once -fgcse-after-reload -frerun-cse-after-loop -frename-registers

#dingux
dingux.mipsel.toolchain = $(toolchains.root)/mipsel-linux-uclibc
dingux.mipsel.execprefix = $(dingux.mipsel.toolchain)/usr/bin/mipsel-linux-
dingux.mipsel.boost.version = 1.49.0
dingux.mipsel.qt.version = 4.7.1
dingux.mipsel.cxx.flags = -mips32
dingux.mipsel.definitions = 'WCHAR_MIN=(0)' 'WCHAR_MAX=((1<<(8*sizeof(wchar_t)))-1)' 'BOOST_FILESYSTEM_VERSION=2'

#linux.i686
linux.i686.boost.version = 1.49.0
linux.i686.boost.libs.model = -mt
linux.i686.qt.version = 4.8.1
linux.i686.cxx.flags = -march=i686 -m32 -mmmx
linux.i686.ld.flags = -m32

#linux.x86_64
linux.x86_64.boost.version = 1.49.0
linux.x86_64.boost.libs.model = -mt
linux.x86_64.qt.version = 4.8.1
linux.x86_64.cxx.flags = -m64 -mmmx
linux.x86_64.ld.flags = -m64

#linux arm
linux.arm.toolchain = $(toolchains.root)/arm-linux
linux.arm.execprefix = $(linux.arm.toolchain)/bin/arm-none-linux-gnueabi-
linux.arm.boost.version = 1.49.0
linux.arm.boost.libs.model = -mt
linux.arm.qt.version = 4.8.1
linux.arm.crossroot = $(prebuilt.dir)/linux-arm

#linux armhf
linux.armhf.toolchain = $(toolchains.root)/armhf-linux
linux.armhf.execprefix = $(linux.armhf.toolchain)/bin/arm-linux-gnueabihf-
linux.armhf.crossroot = $(prebuilt.dir)/root-linux-armhf
linux.armhf.boost.version = 1.49.0
linux.armhf.qt.version = 4.8.1
linux.armhf.qt.libs = $(linux.armhf.crossroot)/usr/lib/arm-linux-gnueabihf
linux.armhf.cxx.flags = -march=armv6 -mfpu=vfp -mfloat-abi=hard -Wa,--no-warn

#mingw
mingw.toolchain = $(toolchains.root)/MinGW
mingw.execprefix = $(mingw.toolchain)/bin/
mingw.cxx.flags = -mthreads -mwin32 -mno-ms-bitfields -mmmx -msse -msse2
mingw.ld.flags = -mthreads -static -Wl,--allow-multiple-definition
mingw.definitions = BOOST_THREAD_USE_LIB 'BOOST_FILESYSTEM_VERSION=3'
# x86
mingw.x86.execprefix = $(mingw.execprefix)
mingw.x86.boost.version = 1.49.0
mingw.x86.qt.version = 4.8.1
mingw.x86.cxx.flags = -m32
mingw.x86.ld.flags = -m32
# x86_64
mingw.x86_64.execprefix = $(mingw.execprefix)
mingw.x86_64.boost.version = 1.49.0
mingw.x86_64.qt.version = 4.8.1
mingw.x86_64.cxx.flags = -m64
mingw.x86_64.ld.flags = -m64

#windows
# x86
windows.x86.boost.version = 1.49.0
windows.x86.qt.version = 4.8.1
# x86_64
windows.x86_64.boost.version = 1.49.0
windows.x86_64.qt.version = 4.8.1
