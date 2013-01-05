#android
android.ndk = /opt/android-ndk
# x86
android.x86.toolchain = /opt/x86-linux-android
android.x86.execprefix = $(android.x86.toolchain)/bin/i686-linux-android-
android.x86.sysroot = $(android.ndk)/platforms/android-14/arch-x86
android.x86.boost.version = 1.49.0
# armeabi
android.armeabi.toolchain = /opt/armeabi-linux-android
android.armeabi.execprefix = $(android.armeabi.toolchain)/bin/arm-linux-androideabi-
android.armeabi.sysroot = $(android.ndk)/platforms/android-14/arch-arm
android.armeabi.boost.version = 1.49.0
# armeabi-v7a
android.armeabi-v7a.toolchain = $(android.armeabi.toolchain)
android.armeabi-v7a.execprefix = $(android.armeabi.execprefix)
android.armeabi-v7a.sysroot = $(android.armeabi.sysroot)
android.armeabi-v7a.boost.version = 1.49.0
# mips
android.mips.toolchain = /opt/mipsel-linux-android
android.mips.execprefix = $(android.mips.toolchain)/bin/mipsel-linux-android-
android.mips.sysroot = $(android.ndk)/platforms/android-14/arch-mips
android.mips.boost.version = 1.49.0
