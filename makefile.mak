include $(path_step)/make/default.mak

.SUFFIXES:

src_suffix := .cpp
res_suffix := .rc

ifneq ($(or $(pic),$(dynamic_name)),)
pic := 1
suffix := _pic
endif

ifdef release
mode := release
else
mode := debug
endif

#set directories
include_dirs = $(path_step)/include $(path_step)/src $(include_path) $(path_step) 
libs_dir = $(path_step)/lib/$(platform)/$(mode)$(suffix)
objs_dir = $(path_step)/obj/$(platform)/$(mode)$(suffix)
bins_dir = $(path_step)/bin/$(platform)/$(mode)

#set platform-specific parameters
include $(path_step)/make/platforms/$(platform).mak

#set features
include $(path_step)/make/features.mak

#tune output according to type
ifdef library_name
output_dir = $(libs_dir)
objects_dir = $(objs_dir)/$(library_name)
target = $(output_dir)/$(call makelib_name,$(library_name))
else ifdef binary_name
output_dir = $(bins_dir)
objects_dir = $(objs_dir)/$(binary_name)
target = $(output_dir)/$(call makebin_name,$(binary_name))
else ifdef dynamic_name
output_dir = $(bins_dir)
objects_dir = $(objs_dir)/$(dynamic_name)
target = $(output_dir)/$(call makedyn_name,$(dynamic_name))
pic := 1
else
$(error Invalid target)
endif

generated_files += $(text_files:=.h)

#main target
all: $(target)

#set compiler-specific parameters
include $(path_step)/make/compilers/$(compiler).mak

#calculate input source files
ifdef source_dirs
source_files += $(basename $(wildcard $(addsuffix /*$(src_suffix),$(source_dirs))))
endif

#process texts if required
ifdef text_files
dep_texts = $(other_text_files:=.txt)
source_files += $(text_files)

#textator can be get from
#http://code.google.com/p/textator
#if were no changes in txt files, just touch .h and .cpp files in this folder or change TEXTATOR to true
TEXTATOR := textator
TEXTATOR_FLAGS := --verbose --process --cpp --symboltype "Char" --memtype "extern const" --tab 2 --width 112

%$(src_suffix): %.txt $(dep_texts)
	$(TEXTATOR) $(TEXTATOR_FLAGS) --inline --output $@ $<

%.h: %.txt $(dep_texts)
	$(TEXTATOR) $(TEXTATOR_FLAGS) --noinline --output $@ $<
endif

#calculate object files from sources
object_files += $(foreach src,$(notdir $(source_files)),\
                  $(objects_dir)/$(call makeobj_name,$(src)))

#calculate object files from windows resources
object_files += $(foreach res,$(notdir $($(platform)_resources)),\
		  $(objects_dir)/$(call makeres_name,$(res)))

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
libs_files = $(foreach lib,$(libraries),$(libs_dir)/$(call makelib_name,$(lib)))

$(target): $(object_files) $(libs_files) | $(output_dir)
	$(link_cmd)
	$(postlink_cmd)

$(libs_files): deps

deps: $(depends)

$(depends):
	$(MAKE) -C $(addprefix $(path_step)/,$@) $(if $(pic),pic=1,) $(MAKECMDGOALS)
endif

$(object_files): | $(objects_dir) $(addsuffix $(src_suffix),$(source_files)) $(addsuffix $(res_suffix),$($(platform)_resources)) $(generated_files)

VPATH = $(dir $(source_files) $($(platform)_resources))

$(objects_dir)/%$(call makeobj_name,): %$(src_suffix)
	$(call build_obj_cmd,$(CURDIR)/$<,$@)

$(objects_dir)/%$(call makeres_name,): %$(res_suffix) | $(objects_dir)
	$(call makeres_cmd,$<,$@)

.PHONY: clean clean_all

clean: clean_self clean_deps

clean_self:
	-$(call rmfiles_cmd,$(wildcard $(target)*))
	-$(call rmdir_cmd,$(objects_dir))

clean_deps: $(depends)

include $(path_step)/make/codeblocks.mak

#show some help
help:
	@echo Targets:
	@echo   all - build target (default)
	@echo   clean - clean target with all the dependencies
	@echo   clean_self - clean only target
	@echo   package - create package
	@echo   help - this page
	@echo Accepted flags via flag=value options for make:
	@echo   profile - enable profiling. Default no
	@echo   platform - selected platform. Default '$(platform)'
	@echo   arch - selected architecture. Default is '$(arch)'
	@echo   CXX - used compiler. Default '$(CXX)'
	@echo   cxx_flags - some specific compilation flags. Default '$(cxx_flags)'
	@echo   LD - used linker. Default '$(LD)'
	@echo   ld_flags - some specific linking flags. Default '$(ld_flags)'
	@echo   defines - additional defines.  Default '$(defines)'
