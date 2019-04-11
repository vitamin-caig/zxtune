#apply default values
ifdef arch
ifndef distro
ifndef toolchains.root
$(error No toolchains.root defined)
endif
ifndef prebuilt.dir
$(error No prebuilt.dir defined)
endif
endif
endif

#android
android.boost.version = 1.65.1
android.cxx.flags = -no-canonical-prefixes -funwind-tables -fstack-protector -fomit-frame-pointer -Wa,--noexecstack
android.ld.flags = -no-canonical-prefixes -Wl,-soname,$(notdir $@) -Wl,--no-undefined -Wl,-z,noexecstack -Wl,-z,relro -Wl,-z,now
#assume that all the platforms are little-endian
#this required to use boost which doesn't know anything about __armel__ or __mipsel__
defines.android += ANDROID __ANDROID__ __LITTLE_ENDIAN__ NO_DEBUG_LOGS NO_L10N LITTLE_ENDIAN
# x86
android.x86.toolchain = $(toolchains.root)/i686-linux-android
android.x86.execprefix = $(android.x86.toolchain)/bin/i686-linux-android-
android.x86.boost.version = $(android.boost.version)
defines.android.x86 += __ANDROID_API__=14
android.x86.ld.flags = -Wl,--icf=safe
# armeabi
android.armeabi.toolchain = $(toolchains.root)/arm-linux-androideabi
android.armeabi.execprefix = $(android.armeabi.toolchain)/bin/arm-linux-androideabi-
android.armeabi.boost.version = $(android.boost.version)
defines.android.armeabi += __ANDROID_API__=14
android.armeabi.cxx.flags = -march=armv5te -mtune=xscale -msoft-float -mthumb -finline-limit=64
# armeabi-v7a
android.armeabi-v7a.toolchain = $(android.armeabi.toolchain)
android.armeabi-v7a.execprefix = $(android.armeabi.execprefix)
android.armeabi-v7a.boost.version = $(android.boost.version)
defines.android.armeabi-v7a += __ANDROID_API__=14
android.armeabi-v7a.cxx.flags = -march=armv7-a -mfpu=vfpv3-d16 -mfloat-abi=softfp -mthumb -finline-limit=64
android.armeabi-v7a.ld.flags = -march=armv7-a -Wl,--fix-cortex-a8
# arm64-v8a
android.arm64-v8a.toolchain = $(toolchains.root)/aarch64-linux-android
android.arm64-v8a.execprefix = $(android.arm64-v8a.toolchain)/bin/aarch64-linux-android-
android.arm64-v8a.boost.version = $(android.boost.version)
defines.android.arm64-v8a += __ANDROID_API__=21
# mips
android.mips.toolchain = $(toolchains.root)/mipsel-linux-android
android.mips.execprefix = $(android.mips.toolchain)/bin/mipsel-linux-android-
android.mips.boost.version = $(android.boost.version)
android.mips.cxx.flags = -fno-inline-functions-called-once -fgcse-after-reload -frerun-cse-after-loop -frename-registers

#dingux
dingux.mipsel.toolchain = $(toolchains.root)/mipsel-linux-uclibc
dingux.mipsel.execprefix = $(dingux.mipsel.toolchain)/usr/bin/mipsel-linux-
dingux.mipsel.boost.version = 1.49.0
dingux.mipsel.qt.version = 4.7.1
dingux.mipsel.cxx.flags = -mips32
defines.dingux.mipsel += 'WCHAR_MIN=(0)' 'WCHAR_MAX=((1<<(8*sizeof(wchar_t)))-1)' 'BOOST_FILESYSTEM_VERSION=2' NO_DEBUG_LOGS NO_L10N

#linux.i686
linux.i686.boost.version = 1.58.0
linux.i686.boost.libs.model = -mt
linux.i686.qt.version = 4.8.6
linux.i686.cxx.flags = -march=i686 -m32 -mmmx
linux.i686.ld.flags = -m32

#linux.x86_64
linux.x86_64.boost.version = 1.65.1
linux.x86_64.boost.libs.model = -mt
linux.x86_64.qt.version = 4.8.6
linux.x86_64.cxx.flags = -m64 -mmmx
linux.x86_64.ld.flags = -m64

#linux armhf
linux.armhf.toolchain = $(toolchains.root)/armhf-linux
linux.armhf.execprefix = $(linux.armhf.toolchain)/bin/arm-linux-gnueabihf-
linux.armhf.crossroot = $(prebuilt.dir)/root-linux-armhf
linux.armhf.boost.version = 1.58.0
linux.armhf.qt.version = 4.8.6
linux.armhf.qt.libs = $(linux.armhf.crossroot)/usr/lib/arm-linux-gnueabihf
linux.armhf.cxx.flags = -march=armv6 -mfpu=vfp -mfloat-abi=hard -Wa,--no-warn

#mingw
mingw.toolchain = $(toolchains.root)/MinGW
mingw.execprefix = $(mingw.toolchain)/bin/
mingw.cxx.flags = -mthreads -mwin32 -mno-ms-bitfields -mmmx -msse -msse2
mingw.ld.flags = -mthreads -static -Wl,--allow-multiple-definition
defines.mingw += BOOST_FILESYSTEM_VERSION=3
# x86
mingw.x86.execprefix = $(mingw.execprefix)
mingw.x86.boost.version = 1.55.0
mingw.x86.qt.version = 4.8.5
mingw.x86.cxx.flags = -m32
mingw.x86.ld.flags = -m32
# x86_64
mingw.x86_64.execprefix = $(mingw.execprefix)
mingw.x86_64.boost.version = 1.55.0
mingw.x86_64.qt.version = 4.8.5
mingw.x86_64.cxx.flags = -m64
mingw.x86_64.ld.flags = -m64

#windows
# x86
windows.x86.cxx.flags = /arch:IA32
windows.x86.boost.version = 1.55.0
windows.x86.qt.version = 4.8.5
# x86_64
windows.x86_64.boost.version = 1.55.0
windows.x86_64.qt.version = 4.8.5

#darwin
# x86_64
darwin.x86_64.boost.version = 1.60.0
darwin.x86_64.boost.libs.model = -mt
darwin.x86_64.qt.version = 4.8.6
#darwin.x86_64.cxx.flags = -m64 -mmmx
#darwin.x86_64.ld.flags = -m64
