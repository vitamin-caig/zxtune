#set default parameters
ifdef platform
include $(path_step)/$(platform).mak
else
include $(path_step)/linux.mak
endif

mode := $(if $(mode),$(mode),release)
arch := $(if $(arch),$(arch),native)
CXX := $(if $(CXX),$(CXX),g++)
LDD := $(if $(LDD),$(LDD),g++)

ifneq ($(or $(pic),$(dynamic_name)),)
pic := 1
suffix := _pic
endif

#set directories
include_dirs := $(path_step) $(path_step)/include $(include_path)
libs_dir := $(path_step)/lib/$(mode)$(suffix)
objs_dir := $(path_step)/obj/$(mode)$(suffix)
bins_dir := $(path_step)/bin/$(mode)

#set options according to mode
ifeq ($(mode),release)
cxx_mode_flags := -O3 -DNDEBUG
else ifeq ($(mode),debug)
cxx_mode_flags := -O0
else
$(error Invalid mode)
endif

ifdef profile
cxx_mode_flags := $(cxx_mode_flags) -pg
ld_mode_flags := $(ld_mode_flags) -pg
endif

#tune output according to type
ifdef library_name
output_dir := $(libs_dir)
objects_dir := $(objs_dir)/$(library_name)
target := $(output_dir)/$(call makelib_name,$(library_name))
else ifdef binary_name
output_dir := $(bins_dir)
objects_dir := $(objs_dir)/$(binary_name)
target := $(output_dir)/$(call makebin_name,$(binary_name))
else ifdef dynamic_name
output_dir := $(bins_dir)
objects_dir := $(objs_dir)/$(dynamic_name)
target := $(output_dir)/$(call makedyn_name,$(dynamic_name))
ld_mode_flags := $(ld_mode_flags) -shared
pic := 1
else
$(error Invalid target)
endif

ifdef source_dirs
source_files := $(wildcard $(addsuffix /*.cpp,$(source_dirs)))
else ifdef source_files
source_files := $(source_files:=.cpp)
else
$(error Not source_dirs or source_files defined at all)
endif

object_files := $(notdir $(source_files))
object_files := $(addprefix $(objects_dir)/,$(object_files:.cpp=.o))

CXX_FLAGS := $(cxx_mode_flags) $(cxx_flags) $(if $(pic),-fPIC,) -g3 -D__STDC_CONSTANT_MACROS $(addprefix -D, $(definitions)) \
	    -march=$(arch) \
	    -funroll-loops -funsigned-char -fno-strict-aliasing \
	    -W -Wall -ansi -pipe \
	    $(addprefix -I, $(include_dirs)) $(addprefix -D, $(definitions))

AR_FLAGS := cru
LD_FLAGS := $(ld_mode_flags) $(ld_flags) -pipe

LD_SOLID_BEFORE := -Wl,--whole-archive
LD_SOLID_AFTER := -Wl,--no-whole-archive

all: deps dirs $(target)

.PHONY: deps $(depends)

deps: $(depends)

dirs:
	test -d $(objects_dir) || mkdir -p $(objects_dir)
	test -d $(output_dir) || mkdir -p $(output_dir)

$(depends):
	$(MAKE) -C $(addprefix $(path_step)/,$@) $(if $(pic),pic=1,)

ifdef library_name
$(target): $(object_files)
	ar $(AR_FLAGS) $@ $^
else
#binary and dynamic libraries
#put solid libraries to an end
$(target): $(object_files) $(foreach lib,$(libraries),$(libs_dir)/$(call makelib_name,$(lib)))
	   $(LDD) $(LD_FLAGS) -o $@ $(object_files) \
	   -L$(libs_dir) $(addprefix -l, $(libraries)) \
	   -L$(output_dir) $(addprefix -l, $(dynamic_libs)) \
	   $(LD_SOLID_BEFORE) $(addprefix -l,$(solid_libs)) $(LD_SOLID_AFTER)
	   objcopy --only-keep-debug $@ $@.pdb
	   strip $@
	   objcopy --add-gnu-debuglink=$@.pdb $@
endif

VPATH := $(source_dirs)

$(objects_dir)/%.o: %.cpp
	$(CXX) $(CXX_FLAGS) -c -MD $< -o $@

clean: clean_deps
	rm -f $(target)
	rm -Rf $(objects_dir)

.PHONY: clean_deps

clean_deps:
	$(foreach dep,$(depends),$(MAKE) -C $(path_step)/$(dep) clean &&) exit 0

#show some help
help:
	@echo "Targets:"
	@echo " all - build target (default)"
	@echo " clean - clean all"
	@echo " help - this page"
	@echo "Accepted flags via flag=value options for make:"
	@echo " mode - compilation mode (release,debug). Default is 'release'"
	@echo " profile - enable profiling. Default no"
	@echo " arch - selected architecture. Default is 'native'"
	@echo " platform - selected platform. Default is 'linux'"
	@echo " CXX - used compiler. Default is 'g++'"
	@echo " cxx_flags - some specific compilation flags"
	@echo " LD - used linker. Default is 'g++'"
	@echo " ld_flags - some specific linking flags"

include $(wildcard $(objects_dir)/*.d)
