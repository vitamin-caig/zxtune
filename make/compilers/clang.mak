#basic definitions for tools
tools.cxx ?= $($(platform).$(arch).execprefix)clang++
tools.cc ?= $($(platform).$(arch).execprefix)clang
tools.ld ?= $($(platform).$(arch).execprefix)clang++
tools.ar ?= $($(platform).$(arch).execprefix)ar
tools.objcopy ?= $($(platform).$(arch).execprefix)echo # STUB!
tools.strip ?= $($(platform).$(arch).execprefix)strip

LINKER_BEGIN_GROUP ?= -Wl,'-('
LINKER_END_GROUP ?= -Wl,'-)'

#set options according to mode
ifdef release
CXX_MODE_FLAGS = -O2 -DNDEBUG -funroll-loops
else
CXX_MODE_FLAGS = -O0
endif

#setup profiling
ifdef profile
CXX_MODE_FLAGS += -pg
LD_MODE_FLAGS += -pg
else
CXX_MODE_FLAGS += -fdata-sections -ffunction-sections
endif

#setup PIC code
ifdef pic
CXX_MODE_FLAGS += -fPIC
LD_MODE_FLAGS += -shared
endif

#setup code coverage
ifdef coverage
ifdef release
$(error Code coverage is not supported in release mode)
endif
CXX_MODE_FLAGS += --coverage
LD_MODE_FLAGS += --coverage
endif

DEFINITIONS = $(defines) $($(platform)_definitions)
INCLUDES = $(sort $(include_dirs) $($(platform)_include_dirs))
INCLUDE_FILES = $(include_files) $($(platform)_include_files)

linux.cxx.flags += -stdlib=libstdc++
linux.ld.flags += -stdlib=libstdc++

darwin.cxx.flags += -stdlib=libc++
darwin.ld.flags += -stdlib=libc++

#setup flags
CCFLAGS = -g $(CXX_MODE_FLAGS) $(cxx_flags) $($(platform).cxx.flags) $($(platform).$(arch).cxx.flags) \
	$(addprefix -D,$(DEFINITIONS) $($(platform).definitions) $($(platform).$(arch).definitions)) \
	-funsigned-char -fno-strict-aliasing \
	-W -Wall -Wextra -pipe \
	$(addprefix -I,$(INCLUDES)) $(addprefix -include ,$(INCLUDE_FILES))

CXXFLAGS = $(CCFLAGS) -std=c++11 -fvisibility=hidden -fvisibility-inlines-hidden

ARFLAGS := crus
LDFLAGS = $(LD_MODE_FLAGS) $($(platform).ld.flags) $($(platform).$(arch).ld.flags) $(ld_flags)

#specify endpoint commands
build_obj_cmd_nodeps = $(tools.cxx) $(CXXFLAGS) -c $1 -o $2
build_obj_cmd = $(build_obj_cmd_nodeps) -MMD
build_obj_cmd_cc = $(tools.cc) $(CCFLAGS) -c $1 -o $2
build_lib_cmd = $(tools.ar) $(ARFLAGS) $2 $1
link_cmd = $(tools.ld) $(LDFLAGS) -o $@ $(OBJECTS) $(RESOURCES) \
	$(if $(libraries),-L$(libs_dir)\
          $(LINKER_BEGIN_GROUP) $(addprefix -l,$(libraries)) $(LINKER_END_GROUP),)\
        $(addprefix -L,$($(platform)_libraries_dirs))\
        $(LINKER_BEGIN_GROUP) $(addprefix -l,$(sort $($(platform)_libraries))) $(LINKER_END_GROUP)\
	$(if $(dynamic_libs),-L$(output_dir) $(addprefix -l,$(dynamic_libs)),)

#include generated dependensies
include $(wildcard $(objects_dir)/*.d)
