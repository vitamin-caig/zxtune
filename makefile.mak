#set default parameters
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
else ifeq ($(mode),profile)
cxx_mode_flags := -O3 -pg -DNDEBUG
ld_mode_flags := -pg
else ifeq ($(mode),profdebug)
cxx_mode_flags := -O0 -pg
ld_mode_flags := -pg
else
$(error Invalid mode)
endif

#tune output according to type
ifdef library_name
output_dir := $(libs_dir)
objects_dir := $(objs_dir)/$(library_name)
target := $(output_dir)/lib$(library_name).a
else ifdef binary_name
output_dir := $(bins_dir)
objects_dir := $(objs_dir)/$(binary_name)
target := $(output_dir)/$(binary_name)
else ifdef dynamic_name
output_dir := $(bins_dir)
objects_dir := $(objs_dir)/$(dynamic_name)
target := $(output_dir)/lib$(dynamic_name).so
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

CXX_FLAGS := $(cxx_mode_flags) $(cxx_flags) $(if $(pic),-fPIC,) -g3 -D__STDC_CONSTANT_MACROS -march=$(arch) \
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
$(target): $(object_files) $(addprefix $(libs_dir)/lib, $(addsuffix .a, $(libraries)))
	   $(LDD) $(LD_FLAGS) -o $@ $(object_files) \
	   -L$(libs_dir) $(addprefix -l, $(libraries)) \
	   -L$(output_dir) $(addprefix -l, $(dynamic_libs)) \
	   $(LD_SOLID_BEFORE) $(addprefix -l,$(solid_libs)) $(LD_SOLID_AFTER)
	   test -e $@ && \
	   objcopy --only-keep-debug $@ $@.pdb && \
	   strip $@ && \
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
	@echo " mode - compilation mode (release,debug,profile,profdebug). Default is 'release'"
	@echo " arch - selected architecture. Default is 'native'"
	@echo " CXX - used compiler. Default is 'g++'"
	@echo " cxx_flags - some specific compilation flags"
	@echo " LD - used linker. Default is 'g++'"
	@echo " ld_flags - some specific linking flags"

include $(wildcard $(objects_dir)/*.d)
