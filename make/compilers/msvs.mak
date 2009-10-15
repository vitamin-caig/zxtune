CXX := cl.exe
LDD := link.exe
AR := lib.exe

#set options according to mode
ifeq ($(mode),release)
cxx_mode_flags := /Ox /DNDEBUG /MD
ld_mode_flags := msvcrt.lib msvcprt.lib
else ifeq ($(mode),debug)
cxx_mode_flags := /Od /MDd
ld_mode_flags := msvcrtd.lib msvcprtd.lib
else
$(error Invalid mode)
endif

ifdef pic
cxx_mode_flags := $(cxx_mode_flags) /LD
ld_mode_flags := $(ld_mode_flags) /DLL
endif

CXX_FLAGS := $(cxx_mode_flags) $(cxx_flags) /FC /TP /nologo /Gy \
	/W3 /Wp64 /wd4224 /wd4710 /wd4711 \
	/D_SECURE_SCL=0 \
	$(addprefix /D, $(definitions)) \
	/J /Zc:wchar_t,forScope /Zi /Za /EHsc /GR \
	$(addprefix /I, $(include_dirs))

LD_FLAGS := $(ld_mode_flags) /NODEFAULTLIB

LD_SOLID_BEFORE := /OPT:NOREF
LD_SOLID_AFTER := /OPT:REF

build_obj_cmd = $(CXX) $(CXX_FLAGS) /c /Fo$@ $<
build_lib_cmd = $(AR) /NOLOGO /OUT:$@ $^
link_cmd = $(LDD) $(LD_FLAGS) /NOLOGO /INCREMENTAL:NO /FIXED:NO /DEBUG \
        /OUT:$@ $(object_files) \
	kernel32.lib delayimp.lib \
	/LIBPATH:$(libs_dir) $(addsuffix .lib,$(libraries)) \
	/LIBPATH:$(output_dir) $(addprefix /DELAYLOAD:,$(addsuffix .dll,$(dynamic_libs))) \
	$(addsuffix .lib,$(dynamic_libs)) \
	$(LD_SOLID_BEFORE) $(addsuffix .lib,$(solid_libs)) $(LD_SOLID_AFTER) \
	/PDB:$@.pdb

#postlink_cmd = 
