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

#setup environment
definitions += $(defines) __STDC_CONSTANT_MACROS

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

generated_files += $(text_files:=.h) $(text_files:=.cpp)

#main target
all: $(target) | $(generated_files)

#set compiler-specific parameters
include $(path_step)/make/compilers/$(compiler).mak

#calculate input source files
ifdef source_dirs
source_files := $(wildcard $(addsuffix /*.cpp,$(source_dirs)))
else ifdef source_files
source_files := $(source_files:=.cpp)
else ifeq ($(or $(text_files),$(ui_files)),)
$(error Not source_dirs or source_files or text_files or ui_files defined at all)
endif

#process texts if required
ifdef text_files
dep_texts := $(other_text_files:=.txt)
source_files += $(text_files:=.cpp)

#textator can be get from
#http://code.google.com/p/textator
#if were no changes in txt files, just touch .h and .cpp files in this folder or change TEXTATOR to true
TEXTATOR := textator
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
$(objects_dir):
	$(call makedir_cmd,$@)

$(output_dir):
	$(info Building $(if $(library_name),library $(library_name),\
	$(if $(binary_name),executable $(binary_name),dynamic object $(dynamic_name))))
	$(call makedir_cmd,$@)

#build target
ifdef library_name
#simple libraries
$(target): $(object_files) | $(output_dir)
	$(call build_lib_cmd,$^,$@)
else
#binary and dynamic libraries with dependencies
libs_files := $(foreach lib,$(libraries),$(libs_dir)/$(call makelib_name,$(lib)))

$(target): $(object_files) $(libs_files) | $(output_dir)
	$(link_cmd)
	$(postlink_cmd)

$(libs_files): deps

deps: $(depends)

$(depends):
	$(MAKE) -C $(addprefix $(path_step)/,$@) $(if $(pic),pic=1,) $(MAKECMDGOALS)
endif

$(object_files): | $(objects_dir)

VPATH := $(dir $(source_files))

$(objects_dir)/%$(call makeobj_name,): %.cpp
	$(call build_obj_cmd,$(CURDIR)/$<,$@)

.PHONY: clean clean_all

clean: clean_self clean_deps

clean_self:
	-$(call rmfiles_cmd,$(wildcard $(target)*))
	-$(call rmdir_cmd,$(objects_dir))

clean_deps: $(depends)

include $(path_step)/codeblocks.mak
include $(path_step)/package.mak

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
