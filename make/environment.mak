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
