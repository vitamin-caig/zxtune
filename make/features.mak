ifdef support_directsound
windows_libraries += user32
endif

ifdef support_zlib
$(platform)_definitions += ZLIB_SUPPORT
$(platform)_libraries += z
windows_include_dirs += $(path_step)/3rdparty/zlib
windows_depends += 3rdparty/zlib
endif
