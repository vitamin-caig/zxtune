makebin_name = $(1).exe
makelib_name = lib$(1).a
makedyn_name = $(1).dll
makeobj_name = $(1).o
makeres_cmd = $(mingw.$(arch).execprefix)windres -O coff --input $(1) --output $(2) $(addprefix -D,$(DEFINES)) -DMANIFEST_NAME=$(platform).manifest
ifeq ($(arch),x86_64)
makeres_cmd += -F pe$(if $(pic),i,)-x86-64
else ifeq ($(arch),x86)
makeres_cmd += -F pe$(if $(pic),i,)-i386
endif

host ?= windows
compiler ?= gcc

ifdef release
$(platform).ld.flags += -Wl,-subsystem,$(if $(have_gui),windows,console)
else
$(platform).ld.flags += -Wl,-subsystem,console
endif
