ifdef support_directsound
windows_libraries += user32
endif

#TODO: extract to dependend lib
$(platform)_libraries += z
$(platform)_depends += 3rdparty/zlib

$(platform)_libraries += lhasa
windows_libraries += oldnames
$(platform)_depends += 3rdparty/lhasa/lib
