include_dirs := $(path_step) $(path_step)/include/boost/tr1/tr1 $(path_step)/include/boost $(path_step)/include include $(include_path)
libs_dir := $(path_step)/lib
objs_dir := $(path_step)/obj

ifdef library_name
output_dir := $(libs_dir)
objects_dir := $(objs_dir)/$(library_name)
target := $(output_dir)/lib$(library_name).a
endif

ifdef binary_name
output_dir := $(path_step)/bin
objects_dir := $(objs_dir)/$(binary_name)
target := $(output_dir)/$(binary_name)
endif

source_files := $(wildcard $(addsuffix /*.cpp,$(source_dirs))) $(cpp_texts)
object_files := $(notdir $(source_files))
object_files := $(addprefix $(objects_dir)/,$(object_files:.cpp=.o))

#TODO: autoconfigure
ARCH := k8

CXX := g++

CXX_FLAGS := -O3 -g1 -DNDEBUG -D__STDC_CONSTANT_MACROS -march=$(ARCH) \
	    -funroll-loops -funsigned-char -fno-strict-aliasing \
	    -W -Wall -ansi -pthread -pipe \
	    $(addprefix -I, $(include_dirs)) $(addprefix -D, $(definitions))

AR_FLAGS := cru
LD_FLAGS := -pipe

LD_SOLID_BEFORE := -Wl,--whole-archive
LD_SOLID_AFTER := -Wl,--no-whole-archive

all: deps dirs $(target)

.PHONY: deps $(depends)

deps: $(depends)

dirs:
	mkdir -p $(objects_dir)
	mkdir -p $(output_dir)

$(depends):
	$(MAKE) -C $(addprefix $(path_step)/,$@)

ifdef binary_name
#put solid libraries to an end
$(target): $(object_files) $(addprefix $(libs_dir)/lib, $(addsuffix .a, $(libraries)))
	   $(CXX) $(LD_FLAGS) -o $@ $(object_files) \
	   -L$(libs_dir) $(addprefix -l, $(libraries)) \
	   $(addprefix -l, $(dynamic_libs)) \
	   $(LD_SOLID_BEFORE) $(addprefix -l,$(solid_libs)) $(LD_SOLID_AFTER)
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
