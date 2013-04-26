makebin_name = $(1).exe
makelib_name = lib$(1).a
makedyn_name = $(1).dll
makeobj_name = $(1).o
makeres_name = $(1).res
makeres_cmd = $(mingw.execprefix)windres -O coff --input $(1) --output $(2) $(addprefix -D,$(DEFINITIONS))
ifeq ($(arch),x86_64)
arch64 := 1
makeres_cmd += -F pe$(if $(pic),i,)-x86-64 -O coff -DMANIFEST_NAME=$(platform)_$(arch).manifest
else
makeres_cmd += -F pe$(if $(pic),i,)-i386
endif

host=windows
compiler=gcc

ifdef have_gui
$(platform).ld.flags += -Wl,-subsystem,windows
else
$(platform).ld.flags += -Wl,-subsystem,console
endif

#simple library naming convention used
mingw_libraries += $(foreach lib,$(libraries.boost),boost_$(lib)-mt$(if $(release),,-d))

mingw_libraries += $(foreach lib,$(libraries.qt),Qt$(lib)$(if $(release),,d))

