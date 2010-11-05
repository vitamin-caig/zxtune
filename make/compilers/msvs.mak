CXX := cl.exe
LDD := link.exe
AR := lib.exe

#set options according to mode
ifdef release
CXX_MODE_FLAGS = /Ox /DNDEBUG /MD
LD_MODE_FLAGS = msvcrt.lib msvcprt.lib /SUBSYSTEM:$(if $(qt_libraries),WINDOWS,CONSOLE)
else
CXX_MODE_FLAGS = /Od /MDd
LD_MODE_FLAGS = msvcrtd.lib msvcprtd.lib /SUBSYSTEM:CONSOLE
endif

ifdef pic
CXX_MODE_FLAGS += /LD
LD_MODE_FLAGS += /DLL
endif

#setup flags
CXXFLAGS := /nologo /c $(CXX_PLATFORM_FLAGS) $(CXX_MODE_FLAGS) \
	/W3 \
	/D_SCL_SECURE_NO_WARNINGS \
	$(addprefix /D, $(definitions)) \
	/J /Zc:wchar_t,forScope /Z7 /Zl /EHsc \
	/GA /GF /Gy /Y- \
	$(addprefix /I, $(include_dirs))

ARFLAGS = /NOLOGO /NODEFAULTLIB

LDFLAGS = /NOLOGO $(LD_PLATFORM_FLAGS) $(LD_MODE_FLAGS) \
	/INCREMENTAL:NO /DEBUG\
	/IGNORE:4217 /IGNORE:4049\
	/OPT:REF,NOWIN98,ICF=5 /NODEFAULTLIB

build_obj_cmd = $(CXX) $(CXXFLAGS) /Fo$2 $1
build_lib_cmd = $(AR) $(ARFLAGS) /OUT:$2 $1
#ignore some warnings for Qt
link_cmd = $(LDD) $(LDFLAGS) /OUT:$@ $(object_files) \
	kernel32.lib $(addsuffix .lib,$(windows_libraries))\
	$(if $(libraries),/LIBPATH:$(libs_dir) $(addsuffix .lib,$(libraries)),)\
	$(if $(dynamic_libs),/LIBPATH:$(output_dir) $(addprefix /DELAYLOAD:,$(addsuffix .dll,$(dynamic_libs))) $(addsuffix .lib,$(dynamic_libs)),)\
	/PDB:$@.pdb

postlink_cmd = mt.exe -manifest $@.manifest -outputresource:$@ || ECHO No manifest
