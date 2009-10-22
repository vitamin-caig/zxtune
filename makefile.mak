#set default parameters
platform := $(if $(platform),$(platform),linux)

mode := $(if $(mode),$(mode),debug)

ifneq ($(or $(pic),$(dynamic_name)),)
pic := 1
suffix := _pic
endif

#set directories
include_dirs := $(path_step)/include $(path_step)/src $(path_step) $(include_path)
libs_dir := $(path_step)/lib/$(mode)$(suffix)
objs_dir := $(path_step)/obj/$(mode)$(suffix)
bins_dir := $(path_step)/bin/$(mode)

include $(path_step)/make/platforms/$(platform).mak

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

#setup environment
definitions := $(definitions) __STDC_CONSTANT_MACROS

include $(path_step)/make/compilers/$(compiler).mak

ifdef source_dirs
source_files := $(wildcard $(addsuffix /*.cpp,$(source_dirs)))
else ifdef source_files
source_files := $(source_files:=.cpp)
else
$(error Not source_dirs or source_files defined at all)
endif

object_files := $(notdir $(source_files))
object_files := $(addprefix $(objects_dir)/,$(object_files:.cpp=$(call makeobj_name,)))

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
	$(build_lib_cmd)
else
#binary and dynamic libraries
#put solid libraries to an end
$(target): $(object_files) $(foreach lib,$(libraries),$(libs_dir)/$(call makelib_name,$(lib)))
	$(link_cmd)
	$(postlink_cmd)
endif

VPATH := $(source_dirs)

$(objects_dir)/%$(call makeobj_name,): %.cpp
	$(build_obj_cmd)

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
