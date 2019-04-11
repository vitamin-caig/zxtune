makebin_name = $(1).exe
makelib_name = $(1).lib
makedyn_name = $(1).dll
makeobj_name = $(1).obj
makeres_cmd = rc $(addprefix /d, $(DEFINES)) /r /fo$(2) $(1)

host=windows
compiler := msvs

# installable boost names convention used
# [prefix]boost_[lib]-mt[-gd].lib
# prefix - 'lib' for static libraries
# lib - library name
# -gd - used for debug libraries
libraries.windows += $(foreach lib,$(libraries.boost),$(if $(boost_dynamic),,lib)boost_$(lib)-mt$(if $(release),,-gd))

# buildable qt names convention used
# Qt[lib][d][4].lib
# lib - library name
# d - used for debug libraries
# 4 - used for dynamic linkage
libraries.windows += $(foreach lib,$(if $(libraries.qt),$(libraries.qt) main,),Qt$(lib)$(if $(release),,d)$(if $(qt_dynamic),4,))
