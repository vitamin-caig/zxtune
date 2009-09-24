#set default parameters
mode := $(if $(mode),$(mode),release)
arch := $(if $(arch),$(arch),native)
CXX := $(if $(CXX),$(CXX),g++)

#set directories
include_dirs := $(path_step) $(path_step)/include $(include_path)
libs_dir := $(path_step)/lib/$(mode)
objs_dir := $(path_step)/obj/$(mode)
bins_dir := $(path_step)/bin/$(mode)

#tune output according to type
ifdef library_name
output_dir := $(libs_dir)
objects_dir := $(objs_dir)/$(library_name)
target := $(output_dir)/lib$(library_name).a
else ifdef binary_name
output_dir := $(bins_dir)
objects_dir := $(objs_dir)/$(binary_name)
target := $(output_dir)/$(binary_name)
else
$(error Invalid target)
endif

source_files := $(wildcard $(addsuffix /*.cpp,$(source_dirs))) $(cpp_texts)
object_files := $(notdir $(source_files))
object_files := $(addprefix $(objects_dir)/,$(object_files:.cpp=.o))

#set options according to mode
ifeq ($(mode),release)
compiler_flags := -O3 -DNDEBUG
else ifeq ($(mode),debug)
compiler_flags := -O0
else ifeq ($(mode),profile)
compiler_flags := -O3 -pg -DNDEBUG
linker_flags := -pg
else ifeq ($(mode),profdebug)
compiler_flags := -O0 -pg
linker_flags := -pg
else
$(error Invalid mode)
endif

CXX_FLAGS := $(compiler_flags) -g3 -D__STDC_CONSTANT_MACROS -march=$(arch) \
	    -funroll-loops -funsigned-char -fno-strict-aliasing \
	    -W -Wall -ansi -pipe \
	    $(addprefix -I, $(include_dirs)) $(addprefix -D, $(definitions))

AR_FLAGS := cru
LD_FLAGS := $(linker_flags) -pipe

LD_SOLID_BEFORE := -Wl,--whole-archive
LD_SOLID_AFTER := -Wl,--no-whole-archive

all: deps dirs $(target)

.PHONY: deps $(depends)

deps: $(depends)

dirs:
	test -d $(objects_dir) || mkdir -p $(objects_dir)
	test -d $(output_dir) || mkdir -p $(output_dir)

$(depends):
	$(MAKE) -C $(addprefix $(path_step)/,$@)

ifdef binary_name
#put solid libraries to an end
$(target): $(object_files) $(addprefix $(libs_dir)/lib, $(addsuffix .a, $(libraries)))
	   $(CXX) $(LD_FLAGS) -o $@ $(object_files) \
	   -L$(libs_dir) $(addprefix -l, $(libraries)) \
	   $(addprefix -l, $(dynamic_libs)) \
	   $(LD_SOLID_BEFORE) $(addprefix -l,$(solid_libs)) $(LD_SOLID_AFTER)
	   test -e $@ && \
	   objcopy --only-keep-debug $@ $@.pdb && \
	   strip $@ && \
	   objcopy --add-gnu-debuglink=$@.pdb $@
else
$(target): $(object_files)
	ar $(AR_FLAGS) $@ $^
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

include $(wildcard $(objects_dir)/*.d)
