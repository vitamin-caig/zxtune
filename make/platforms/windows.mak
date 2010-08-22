makebin_name = $(1).exe
makelib_name = $(1).lib
makedyn_name = $(1).dll
makeobj_name = $(1).obj
#for cygwin
#makedir_cmd = mkdir -p $(1)
#for other
makedir_cmd = if NOT EXIST $(subst /,\,$(1)) mkdir $(subst /,\,$(1))

compiler := msvs

# built-in features
support_waveout = 1
support_aylpt_dlportio = 1
#support_sdl = 1

# installable boost names convention used
# [prefix]boost_[lib]-[msvs]-mt[-gd]-[version].lib
# prefix - 'lib' for static libraries
# lib - library name
# msvs - version of MSVS used to compile and link
# -gd - used for debug libraries
# version - boost version major_minor
windows_libraries += $(foreach lib,$(boost_libraries),$(if $(boost_dynamic),,lib)boost_$(lib)-$(MSVS_VERSION)-mt$(if $(subst release,,$(mode)),-gd,)-$(BOOST_VERSION))

# buildable qt names convention used
# Qt[lib][d][4].lib
# lib - library name
# d - used for debug libraries
# 4 - used for dynamic linkage
windows_libraries += $(foreach lib,$(if $(qt_libraries),$(qt_libraries) main,),Qt$(lib)$(if $(subst release,,$(mode)),d,)$(if $(qt_dynamic),4,))
