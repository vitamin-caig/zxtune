makebin_name = $(1).exe
makelib_name = $(1).lib
makedyn_name = $(1).dll
makeobj_name = $(1).obj
makeres_cmd = rc $(addprefix /d, $(DEFINES)) /r /fo$(2) $(1)

host=windows
compiler := msvs

# buildable qt names convention used
# Qt[lib][d][4].lib
# lib - library name
# d - used for debug libraries
# 4 - used for dynamic linkage
libraries.windows += $(foreach lib,$(if $(libraries.qt),$(libraries.qt) main,),Qt$(lib)$(if $(release),,d)$(if $(qt_dynamic),4,))
