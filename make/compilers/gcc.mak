#basic definitions for tools
tool.cxx = $($(platform).$(arch).execprefix)g++
tool.cc = $($(platform).$(arch).execprefix)gcc
tool.ld ?= $($(platform).$(arch).execprefix)g++
tool.ar ?= $($(platform).$(arch).execprefix)ar
tool.objcopy ?= $($(platform).$(arch).execprefix)objcopy
tool.strip ?= $($(platform).$(arch).execprefix)strip

LINKER_BEGIN_GROUP ?= -Wl,'-('
LINKER_END_GROUP ?= -Wl,'-)'

#set options according to mode
ifdef release
CXX_MODE_FLAGS = -O2 -DNDEBUG -funroll-loops -finline-functions
else
CXX_MODE_FLAGS = -O0
endif

#setup profiling
ifdef profile
CXX_MODE_FLAGS += -pg
LD_MODE_FLAGS += -pg
else
CXX_MODE_FLAGS += -fdata-sections -ffunction-sections
ifdef release
LD_MODE_FLAGS += -Wl,-O3,-x,--gc-sections,--relax
endif
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

#setup flags
CCFLAGS = -g $(CXX_MODE_FLAGS) $(cxx_flags) $($(platform).cxx.flags) $($(platform).$(arch).cxx.flags) \
	$(addprefix -D,$(DEFINITIONS) $($(platform).definitions) $($(platform).$(arch).definitions)) \
	-funsigned-char -fno-strict-aliasing \
	-W -Wall -Wextra -pipe \
	$(addprefix -I,$(INCLUDES))

CXXFLAGS = $(CCFLAGS) -ansi -fvisibility=hidden -fvisibility-inlines-hidden

ARFLAGS := crus
LDFLAGS = $(LD_MODE_FLAGS) $($(platform).ld.flags) $($(platform).$(arch).ld.flags) $(ld_flags)

#specify endpoint commands
build_obj_cmd_nodeps = $(tool.cxx) $(CXXFLAGS) -c $1 -o $2
build_obj_cmd = $(build_obj_cmd_nodeps) -MMD
build_obj_cmd_cc = $(tool.cc) $(CCFLAGS) -c $1 -o $2
build_lib_cmd = $(tool.ar) $(ARFLAGS) $2 $1
link_cmd = $(tool.ld) $(LDFLAGS) -o $@ $(OBJECTS) $(RESOURCES) \
	$(if $(libraries),-L$(libs_dir)\
          $(LINKER_BEGIN_GROUP) $(addprefix -l,$(libraries)) $(LINKER_END_GROUP),)\
        $(addprefix -L,$($(platform)_libraries_dirs))\
        $(LINKER_BEGIN_GROUP) $(addprefix -l,$(sort $($(platform)_libraries))) $(LINKER_END_GROUP)\
	$(if $(dynamic_libs),-L$(output_dir) $(addprefix -l,$(dynamic_libs)),)

#specify postlink command- generate pdb file
postlink_cmd = $(tool.objcopy) --only-keep-debug $@ $@.pdb && \
	$(tool.objcopy) --strip-all $@ && \
	$(tool.objcopy) --add-gnu-debuglink=$@.pdb $@

#include generated dependensies
include $(wildcard $(objects_dir)/*.d)

.PHONY: analyze analyze_all

analyze:
	@echo "Analyzing $(target)" > coverage.log
	@for i in $(SOURCES);do gcov -lp -o $(objects_dir) $$i >> coverage.log; done
	@echo `pwd`
	@perl $(path_step)/make/compilers/gcc_coverage.pl

analyze_deps: $(depends)

analyze_all: analyze analyze_deps
