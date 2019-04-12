CXX := cl.exe
LDD := link.exe
AR := lib.exe

ifneq ($(and $(have_gui),$(release)),)
windows.ld.subsystem = WINDOWS
else
windows.ld.subsystem = CONSOLE
endif

#support winxp
windows.x86.ld.subsystem.version = 5.01
windows.x86_64.ld.subsystem.version = 5.02

#set options according to mode
ifdef release
mode.suffix=
CXX_MODE_FLAGS = /Ox /DNDEBUG
else
mode.suffix=d
CXX_MODE_FLAGS = /Od
endif

LD_MODE_FLAGS = /SUBSYSTEM:$(windows.ld.subsystem),$(windows.$(arch).ld.subsystem.version)

ifdef pic
CXX_MODE_FLAGS += /LD
LD_MODE_FLAGS += /DLL
endif

#specific
DEFINES = $(defines) $(defines.$(platform)) $(defines.$(platform).$(arch)) _SCL_SECURE_NO_WARNINGS _CRT_SECURE_NO_WARNINGS
INCLUDES_DIRS = $(includes.dirs) $(includes.dirs.$(platform))
INCLUDES_FILES = $(includes.files) $(includes.files.$(platform))
libraries.windows += kernel32

ifdef static_runtime
CXX_MODE_FLAGS += /MT$(mode.suffix)
libraries.windows += $(addsuffix $(mode.suffix), libcmt libcpmt)
else
CXX_MODE_FLAGS += /MD$(mode.suffix)
libraries.windows += $(addsuffix $(mode.suffix), msvcrt msvcprt)
endif

#setup flags
CXXFLAGS = /nologo /c $(CXX_MODE_FLAGS) $(cxx_flags) $($(platform).cxx.flags) $($(platform).$(arch).cxx.flags) \
	/W3 \
	$(addprefix /D, $(DEFINES)) \
	/J /Zc:wchar_t,forScope /Z7 /Zl /EHsc \
	/GA /GF /Gy /Y- /GR \
	$(addprefix /I, $(INCLUDES_DIRS)) $(addprefix /FI , $(INCLUDES_FILES))

ARFLAGS = /NOLOGO /NODEFAULTLIB

LDFLAGS = /NOLOGO $(LD_PLATFORM_FLAGS) $(LD_MODE_FLAGS) $(ld_flags) \
	/INCREMENTAL:NO /DEBUG\
	/IGNORE:4217 /IGNORE:4049\
	/OPT:REF,ICF=5 /NODEFAULTLIB

build_obj_cmd = $(CXX) $(CXXFLAGS) /Fo$2 $1
build_obj_cmd_nodeps = $(build_obj_cmd)
build_obj_cmd_cc = $(build_obj_cmd)
build_lib_cmd = $(AR) $(ARFLAGS) /OUT:$2 $1
#ignore some warnings for Qt
link_cmd = $(LDD) $(LDFLAGS) /OUT:$@ $(OBJECTS) $(RESOURCES) \
        /LIBPATH:$(libraries.dir) $(addsuffix .lib,$(libraries)) \
        $(if $(libraries.dynamic),/LIBPATH:$(output_dir) \
          $(addprefix /DELAYLOAD:,$(addsuffix .dll,$(libraries.dynamic))) $(addsuffix .lib,$(libraries.dynamic)),)\
        $(addprefix /LIBPATH:,$(libraries.dirs.$(platform)))\
        $(addsuffix .lib,$(sort $(libraries.$(platform))))\
        /MANIFEST:EMBED\
	/PDB:$@.pdb

postlink_cmd = IF EXIST $@.manifest mt.exe -manifest $@.manifest -outputresource:$@
