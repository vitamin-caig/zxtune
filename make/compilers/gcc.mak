#basic definitions for tools
CXX ?= g++
LDD ?= g++
AR ?= ar
OBJCOPY ?= objcopy
STRIP ?= strip

LINKER_BEGIN_GROUP ?= -Wl,-(
LINKER_END_GROUP ?= -Wl,-)

#set options according to mode
ifdef release
CXX_MODE_FLAGS = -O2 -DNDEBUG
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
CXX_MODE_FLAGS += --coverage
LD_MODE_FLAGS += --coverage
endif

DEFINITIONS = $(defines) $($(platform)_definitions) __STDC_CONSTANT_MACROS
INCLUDES = $(sort $(include_dirs) $($(platform)_include_dirs))

#setup flags
CXXFLAGS = $(CXX_PLATFORM_FLAGS) $(CXX_MODE_FLAGS) $(cxx_flags) -c -g3 \
	$(addprefix -D,$(DEFINITIONS)) \
	-funroll-loops -funsigned-char -fno-strict-aliasing \
	-W -Wall -Wextra -ansi -pipe \
	$(addprefix -I,$(INCLUDES))

ARFLAGS := cru
LDFLAGS = $(LD_PLATFORM_FLAGS) $(LD_MODE_FLAGS) $(ld_flags)

#specify endpoint commands
build_obj_cmd = $(CXX) $(CXXFLAGS) -MMD $1 -o $2
build_obj_cmd_nodeps = $(CXX) $(CXXFLAGS) $1 -o $2
build_lib_cmd = $(AR) $(ARFLAGS) $2 $1
link_cmd = $(LDD) $(LDFLAGS) -o $@ $(OBJECTS) $(RESOURCES) \
	$(if $(libraries),-L$(libs_dir)\
          $(LINKER_BEGIN_GROUP) $(addprefix -l,$(libraries)) $(LINKER_END_GROUP),)\
        $(addprefix -L,$($(platform)_libraries_dirs))\
        $(LINKER_BEGIN_GROUP) $(addprefix -l,$(sort $($(platform)_libraries))) $(LINKER_END_GROUP)\
	$(if $(dynamic_libs),-L$(output_dir) $(addprefix -l,$(dynamic_libs)),)

#specify postlink command- generate pdb file
postlink_cmd = $(OBJCOPY) --only-keep-debug $@ $@.pdb && \
	$(STRIP) -s $@ && \
	$(OBJCOPY) --add-gnu-debuglink=$@.pdb $@

#include generated dependensies
include $(wildcard $(objects_dir)/*.d)

.PHONY: analyze analyze_all

analyze:
	@echo "Analyzing $(target)" > coverage.log
	@for i in $(source_files);do gcov -lp -o $(objects_dir) $$i >> coverage.log; done
	@echo `pwd`
	@perl $(path_step)/make/compilers/gcc_coverage.pl

analyze_deps: $(depends)

analyze_all: analyze analyze_deps
