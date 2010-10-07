#basic definitions for tools
CXX := $(if $(CXX),$(CXX),g++)
LDD := $(if $(LDD),$(LDD),g++)
AR := $(if $(AR),$(AR),ar)
OBJCOPY := $(if $(OBJCOPY),$(OBJCOPY),objcopy)
STRIP := $(if $(STRIP),$(STRIP),strip)

#set options according to mode
ifeq ($(mode),release)
cxx_mode_flags += -O2 -DNDEBUG
else ifeq ($(mode),debug)
cxx_mode_flags += -O0
else
$(error Invalid mode)
endif

#setup profiling
ifdef profile
cxx_mode_flags += -pg
ld_mode_flags += -pg
else
cxx_mode_flags += -fdata-sections -ffunction-sections 
ld_mode_flags += -Wl,-O3,-x,--gc-sections,--relax
endif

#setup PIC code
ifdef pic
cxx_mode_flags += -fPIC
ld_mode_flags += -shared
endif

#setup code coverage
ifdef coverage
cxx_mode_flags += --coverage
ld_mode_flags += --coverage
endif

#setup flags
CXX_FLAGS := $(cxx_mode_flags) $(cxx_flags) -g3 \
	$(addprefix -D, $(definitions)) \
	-funroll-loops -funsigned-char -fno-strict-aliasing \
	-W -Wall -Wextra -ansi -pipe \
	$(addprefix -I, $(include_dirs) $($(platform)_include_dirs))

AR_FLAGS := cru
LD_FLAGS := $(ld_mode_flags) $(ld_flags)

#specify endpoint commands
build_obj_cmd = $(CXX) $(CXX_FLAGS) -c -MMD $1 -o $2
build_lib_cmd = $(AR) $(AR_FLAGS) $2 $1
link_cmd = $(LDD) $(LD_FLAGS) -o $@ $(object_files) \
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
