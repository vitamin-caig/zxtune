include $(path_step)/default.mak

ifneq ($(or $(pic),$(dynamic_name)),)
pic := 1
suffix := _pic
endif

#set directories
include_dirs := $(path_step)/include $(path_step)/src $(path_step) $(include_path)
libs_dir := $(path_step)/lib/$(platform)/$(mode)$(suffix)
objs_dir := $(path_step)/obj/$(platform)/$(mode)$(suffix)
bins_dir := $(path_step)/bin/$(platform)/$(mode)

#set platform-specific parameters
include $(path_step)/make/platforms/$(platform).mak

#set features
include $(path_step)/features.mak

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
pic := 1
else
$(error Invalid target)
endif

h_texts := $(text_files:=.h)
cpp_texts := $(text_files:=.cpp)
dep_texts := $(other_text_files:=.txt)

#setup environment
definitions = $(defines) __STDC_CONSTANT_MACROS

#main target
all: dirs $(h_texts) $(cpp_texts) $(target)
.PHONY: all dirs

#set compiler-specific parameters
include $(path_step)/make/compilers/$(compiler).mak

#calculate input source files
ifdef source_dirs
source_files := $(wildcard $(addsuffix /*.cpp,$(source_dirs)))
else ifdef source_files
source_files := $(source_files:=.cpp)
else ifndef text_files
$(error Not source_dirs or source_files or text_files defined at all)
endif

#process texts if required
ifdef text_files
source_files += $(cpp_texts)

#textator can be get from
#http://code.google.com/p/textator
#if were no changes in txt files, just touch .h and .cpp files in this folder or change TEXTATOR to true
TEXTATOR := ~/bin/textator
TEXTATOR_FLAGS := --verbose --process --cpp --symboltype "Char" --memtype "extern const" --tab 2 --width 112

%.cpp: %.txt $(dep_texts)
	$(TEXTATOR) $(TEXTATOR_FLAGS) --inline --output $@ $<

%.h: %.txt $(dep_texts)
	$(TEXTATOR) $(TEXTATOR_FLAGS) --noinline --output $@ $<
endif

#calculate object files from sources
object_files := $(notdir $(source_files))
object_files := $(addprefix $(objects_dir)/,$(object_files:.cpp=$(call makeobj_name,)))

#make objects and binaries dir
dirs:
	@echo Building $(if $(library_name),library $(library_name),\
	  $(if $(binary_name),executable $(binary_name),dynamic object $(dynamic_name)))
	$(call makedir_cmd,$(objects_dir))
	$(call makedir_cmd,$(output_dir))

#build target
ifdef library_name
#simple libraries
$(target): $(object_files)
	$(build_lib_cmd)
else
#binary and dynamic libraries with dependencies
.PHONY: deps $(depends)

deps: $(depends)

$(depends):
	$(MAKE) -C $(addprefix $(path_step)/,$@) $(if $(pic),pic=1,) $(MAKECMDGOALS)

$(target): deps $(object_files) $(foreach lib,$(libraries),$(libs_dir)/$(call makelib_name,$(lib)))
	$(link_cmd)
	$(postlink_cmd)
endif

VPATH := $(source_dirs)

$(objects_dir)/%$(call makeobj_name,): %.cpp
	$(build_obj_cmd)

.PHONY: clean clean_all

clean:
	rm -f $(target)
	rm -Rf $(objects_dir)

clean_all: clean clean_deps

clean_deps: $(depends)

#show some help
help:
	@echo "Targets:"
	@echo " all - build target (default)"
	@echo " clean - clean all"
	@echo " help - this page"
	@echo "Accepted flags via flag=value options for make:"
	@echo " mode - compilation mode (release,debug). Default is 'release'"
	@echo " profile - enable profiling. Default no"
	@echo " platform - selected platform. Default is 'linux'"
	@echo " CXX - used compiler. Default is 'g++'"
	@echo " cxx_flags - some specific compilation flags"
	@echo " LD - used linker. Default is 'g++'"
	@echo " ld_flags - some specific linking flags"
	@echo " defines - additional defines"
