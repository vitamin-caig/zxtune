makebin_name = $(1).exe
makelib_name = $(1).lib
makedyn_name = $(1).dll
makeobj_name = $(1).obj
makeres_name = $(1).res
makeres_cmd = rc $(addprefix /d, $(DEFINITIONS)) /r /fo$(2) $(1)

host=windows
compiler := msvs

# built-in features
support_waveout = 1
support_aylpt_dlportio = 1
support_directsound = 1
#support_sdl = 1
support_mp3 = 1
support_ogg = 1
support_flac = 1

windows_libraries += oldnames

# installable boost names convention used
# [prefix]boost_[lib]-mt[-gd].lib
# prefix - 'lib' for static libraries
# lib - library name
# -gd - used for debug libraries
windows_libraries += $(foreach lib,$(boost_libraries),$(if $(boost_dynamic),,lib)boost_$(lib)-mt$(if $(release),,-gd))

# buildable qt names convention used
# Qt[lib][d][4].lib
# lib - library name
# d - used for debug libraries
# 4 - used for dynamic linkage
windows_libraries += $(foreach lib,$(if $(qt_libraries),$(qt_libraries) main,),Qt$(lib)$(if $(release),,d)$(if $(qt_dynamic),4,))
