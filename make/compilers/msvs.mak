CXX := cl.exe
LDD := link.exe
AR := lib.exe

#set options according to mode
ifeq ($(mode),release)
cxx_mode_flags += /Ox /DNDEBUG /MD
ld_mode_flags += msvcrt.lib msvcprt.lib /SUBSYSTEM:$(if $(qt_libraries),WINDOWS,CONSOLE)
else ifeq ($(mode),debug)
cxx_mode_flags += /Od /MDd
ld_mode_flags += msvcrtd.lib msvcprtd.lib /SUBSYSTEM:CONSOLE
else
$(error Invalid mode)
endif

ifdef pic
cxx_mode_flags += /LD
ld_mode_flags += /DLL
endif

cxx_mode_flags += \
	/W3 \
	/D_SCL_SECURE_NO_WARNINGS \
	$(addprefix /D, $(definitions)) \
	/J /Zc:wchar_t,forScope /Z7 /Zl /EHsc \
	/GA /GF /Gy /Y- \
	$(addprefix /I, $(include_dirs))

build_obj_cmd = $(CXX) $(cxx_flags) $(cxx_mode_flags) /nologo /c /Fo$@ $<
build_lib_cmd = $(AR) /NOLOGO /NODEFAULTLIB /OUT:$@ $^
#ignore some warnings for Qt
link_cmd = $(LDD) $(ld_mode_flags) /NOLOGO /INCREMENTAL:NO /DEBUG \
	/IGNORE:4217 /IGNORE:4049 \
	/OPT:REF,NOWIN98,ICF=5 /NODEFAULTLIB \
	/OUT:$@ $(object_files) \
	kernel32.lib $(addsuffix .lib,$(windows_libraries)) \
	$(if $(libraries),/LIBPATH:$(libs_dir) $(addsuffix .lib,$(libraries)),) \
	$(if $(dynamic_libs),/LIBPATH:$(output_dir) $(addprefix /DELAYLOAD:,$(addsuffix .dll,$(dynamic_libs))) $(addsuffix .lib,$(dynamic_libs)),) \
	/PDB:$@.pdb

postlink_cmd = mt.exe -manifest $@.manifest -outputresource:$@ || ECHO No manifest
