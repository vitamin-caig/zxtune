ifdef support_directsound
windows_libraries += user32
endif

ifdef support_zlib
$(platform)_definitions += ZLIB_SUPPORT
$(platform)_libraries += z
$(platform)_include_dirs += $(path_step)/3rdparty/zlib
$(platform)_depends += 3rdparty/zlib
endif

#TODO: extract to dependend lib
$(platform)_libraries += lhasa
$(platform)_depends += 3rdparty/lhasa/lib
