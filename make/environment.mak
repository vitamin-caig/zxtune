#apply default values
ifdef arch
ifndef distro
ifndef prebuilt.dir
$(error No prebuilt.dir defined)
endif
endif
endif

#generic defines
defines += HAVE_STDINT_H SOURCES_ROOT=\"$(abspath $(dirs.root))\"

#android
# All assembler-related options (e.g. -Wa,--noexecstack) are not applicable for lto mode.
android.cxx.flags = -no-canonical-prefixes -funwind-tables -fstack-protector-strong -fomit-frame-pointer -faddrsig -flto -Wno-gnu-string-literal-operator-template
android.ld.flags = -no-canonical-prefixes -Wl,-soname,$(notdir $@) -Wl,--no-undefined -Wl,-z,noexecstack -Wl,-z,relro -Wl,-z,now -static-libstdc++ -fuse-ld=lld -flto -Wl,--icf=all
#assume that all the platforms are little-endian
#this required to use boost which doesn't know anything about __armel__ or __mipsel__
defines.android += ANDROID __ANDROID__ __LITTLE_ENDIAN__ NO_DEBUG_LOGS NO_L10N LITTLE_ENDIAN

#mingw
mingw.cxx.flags = -mno-ms-bitfields
mingw.ld.flags = -static -Wl,--allow-multiple-definition
mingw.x86.cxx.flags = -mtune=i686

#windows
# x86
windows.x86.cxx.flags = /arch:IA32
# x86_64
