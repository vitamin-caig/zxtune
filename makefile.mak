include_dirs := $(path_step)/include/boost/tr1/tr1 $(path_step)/include/boost $(path_step)/include include $(include_path)
libs_dir := $(path_step)/libs
objs_dir := $(path_step)/objs

ifdef library_name
output_dir := $(libs_dir)
objects_dir := $(objs_dir)/$(library_name)
target := $(output_dir)/lib$(library_name).a
endif

ifdef binary_name
output_dir := $(path_step)/bins
objects_dir := $(objs_dir)/$(binary_name)
target := $(output_dir)/$(binary_name)
endif

source_files := $(wildcard $(addsuffix /*.cpp,$(source_dirs))) $(cpp_texts)
object_files := $(notdir $(source_files))
object_files := $(addprefix $(objects_dir)/,$(object_files:.cpp=.o))

CXX_FLAGS := -O2 -g0 -s -Wall -Wextra -funsigned-char -ansi \
            $(addprefix -I, $(include_dirs)) $(addprefix -D, $(definitions)) -pipe

AR_FLAGS := cru
LD_FLAGS := -pipe

all: deps $(objects_dir) $(output_dir) $(target)

$(objects_dir):
	mkdir -p $(objects_dir)

$(output_dir):
	mkdir -p $(output_dir)

.PHONY: deps $(depends)

deps: $(depends)

$(depends):
	$(MAKE) -C $(addprefix $(path_step)/,$@)

$(target): $(object_files)
ifdef binary_name
	g++ $(LD_FLAGS) -o $@ $^ \
    -Wl,--whole-archive -L$(libs_dir) $(addprefix -l, $(libraries)) -Wl,--no-whole-archive \
    $(addprefix -l, $(dynamic_libs))
else
	ar $(AR_FLAGS) $@ $^
endif

VPATH := $(source_dirs)

$(objects_dir)/%.o: %.cpp
	g++ $(CXX_FLAGS) -c -MD $< -o $@

clean:
	rm -f $(target)
	rm -Rf $(objects_dir)

include $(wildcard *.d)
