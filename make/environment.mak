#apply default values
ifdef arch
ifndef distro
ifndef prebuilt.dir
$(error No prebuilt.dir defined)
endif
endif
endif

#generic defines
defines += HAVE_STDINT_H

#android
# All assembler-related options (e.g. -Wa,--noexecstack) are not applicable for lto mode.
android.cxx.flags = -no-canonical-prefixes -funwind-tables -fstack-protector-strong -fomit-frame-pointer -fno-addrsig -flto
android.ld.flags = -no-canonical-prefixes -Wl,-soname,$(notdir $@) -Wl,--no-undefined -Wl,-z,noexecstack -Wl,-z,relro -Wl,-z,now -static-libstdc++ -fuse-ld=lld -flto
#assume that all the platforms are little-endian
#this required to use boost which doesn't know anything about __armel__ or __mipsel__
defines.android += ANDROID __ANDROID__ __LITTLE_ENDIAN__ NO_DEBUG_LOGS NO_L10N LITTLE_ENDIAN
# x86
android.x86.execprefix = $(android.toolchain)/i686-linux-android-
android.x86.cxx.flags = -m32 -target i686-linux-android16
android.x86.ld.flags = -target i686-linux-android16
# x86_64
android.x86_64.execprefix = $(android.toolchain)/x86_64-linux-android-
android.x86_64.cxx.flags = -m64 -target x86_64-linux-android21
android.x86_64.ld.flags = -target x86_64-linux-android21
# armeabi-v7a
android.armeabi-v7a.execprefix = $(android.toolchain)/arm-linux-androideabi-
android.armeabi-v7a.cxx.flags = -mthumb -target armv7a-linux-androideabi16
android.armeabi-v7a.ld.flags = -mthumb -target armv7a-linux-androideabi16
# arm64-v8a
android.arm64-v8a.execprefix = $(android.toolchain)/aarch64-linux-android-
android.arm64-v8a.cxx.flags = -target aarch64-linux-android21
android.arm64-v8a.ld.flags = -target aarch64-linux-android21

#linux.i686
linux.i686.cxx.flags = -march=i686 -m32 -mmmx
linux.i686.ld.flags = -m32

#linux.x86_64
linux.x86_64.cxx.flags = -m64 -mmmx
linux.x86_64.ld.flags = -m64

#linux armhf
linux.armhf.toolchain = $(toolchains.root)/armhf-linux
linux.armhf.execprefix = $(linux.armhf.toolchain)/bin/arm-linux-gnueabihf-
linux.armhf.crossroot = $(prebuilt.dir)/root-linux-armhf
linux.armhf.qt.libs = $(linux.armhf.crossroot)/usr/lib/arm-linux-gnueabihf
linux.armhf.cxx.flags = -march=armv6 -mfpu=vfp -mfloat-abi=hard -Wa,--no-warn

#mingw
mingw.toolchain ?= $(toolchains.root)/MinGW
mingw.execprefix ?= $(mingw.toolchain)/bin/
mingw.cxx.flags = -mthreads -mwin32 -mno-ms-bitfields -mmmx -msse -msse2
mingw.ld.flags = -mthreads -static -Wl,--allow-multiple-definition
# x86
mingw.x86.execprefix = $(mingw.execprefix)
mingw.x86.cxx.flags = -m32
mingw.x86.ld.flags = -m32
# x86_64
mingw.x86_64.execprefix = $(mingw.execprefix)
mingw.x86_64.cxx.flags = -m64
mingw.x86_64.ld.flags = -m64

#windows
# x86
windows.x86.cxx.flags = /arch:IA32
# x86_64

#darwin
# x86_64
darwin.cxx.flags += -stdlib=libc++
darwin.ld.flags += -stdlib=libc++
