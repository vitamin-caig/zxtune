#basic definitions for tools
CXX := $(if $(CXX),$(CXX),g++)
LDD := $(if $(LDD),$(LDD),g++)
AR := $(if $(AR),$(AR),ar)
OBJCOPY := $(if $(OBJCOPY),$(OBJCOPY),objcopy)
STRIP := $(if $(STRIP),$(STRIP),strip)

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
INCLUDES = $(include_dirs) $($(platform)_include_dirs)

#setup flags
CXXFLAGS = $(CXX_PLATFORM_FLAGS) $(CXX_MODE_FLAGS) $(cxx_flags) -c -g3 \
	$(addprefix -D, $(DEFINITIONS)) \
	-funroll-loops -funsigned-char -fno-strict-aliasing \
	-W -Wall -Wextra -ansi -pipe \
	$(addprefix -I, $(INCLUDES))

ARFLAGS := cru
LDFLAGS = $(LD_PLATFORM_FLAGS) $(LD_MODE_FLAGS) $(ld_flags)

#specify endpoint commands
build_obj_cmd = $(CXX) $(CXXFLAGS) -MMD $1 -o $2
build_obj_cmd_nodeps = $(CXX) $(CXXFLAGS) $1 -o $2
build_lib_cmd = $(AR) $(ARFLAGS) $2 $1
link_cmd = $(LDD) $(LDFLAGS) -o $@ $(OBJECTS) \
	$(if $(libraries),-L$(libs_dir) $(addprefix -l,$(libraries)),) \
	$(if $(dynamic_libs),-L$(output_dir) $(addprefix -l,$(dynamic_libs)),) \
	$(addprefix -L,$($(platform)_libraries_dirs)) $(addprefix -l,$($(platform)_libraries))

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
