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
cxx_mode_flags += /LD
ld_mode_flags += /DLL
endif

CXX_FLAGS := $(cxx_mode_flags) $(cxx_flags) /nologo \
	/W3 /Wp64 /wd4224 /wd4710 /wd4711 \
	/D_SECURE_SCL=0 \
	$(addprefix /D, $(definitions)) \
	/J /Zc:wchar_t,forScope /Fd /Z7 /Zl /EHsc \
	/GA /GL /GF /Gy /Y- \
	$(addprefix /I, $(include_dirs))

LD_FLAGS := $(ld_mode_flags) /NODEFAULTLIB

build_obj_cmd = $(CXX) $(CXX_FLAGS) /c /Fo$@ $<
build_lib_cmd = $(AR) /NOLOGO /NODEFAULTLIB /OUT:$@ $^
link_cmd = $(LDD) $(LD_FLAGS) /NOLOGO /INCREMENTAL:NO /FIXED:NO /DEBUG \
	/OPT:REF,NOWIN98 /NODEFAULTLIB /LTCG \
	/OUT:$@ $(object_files) \
	kernel32.lib $(windows_libraries) \
	$(if $(libraries),/LIBPATH:$(libs_dir) $(addsuffix .lib,$(libraries)),) \
	$(if $(dynamic_libs),/LIBPATH:$(output_dir) $(addprefix /DELAYLOAD:,$(addsuffix .dll,$(dynamic_libs))) $(addsuffix .lib,$(dynamic_libs)),) \
	/PDB:$@.pdb

#postlink_cmd =
