include $(path_step)/make/default.mak

.SUFFIXES:

src_suffix := .cpp
src_suffix_cc := .c
res_suffix := .rc

%$(src_suffix) :

%$(src_suffix_cc) :

%$(res_suffix) :

%.mak :

ifneq ($(or $(pic),$(dynamic_name)),)
pic := 1
suffix := _pic
endif

ifdef release
mode := release
else
mode := debug
endif

ifneq ($(or $(qt_libraries),$(ui_files),$(moc_files),$(qrc_files)),)
use_qt := 1
endif

platform_pathname = $(platform)$(if $(arch),_$(arch),)
mode_pathname = $(mode)$(if $(profile),_profile,)

#set directories
include_dirs += $(path_step)/include $(path_step)/src $(path_step)
objs_dir = $(path_step)/obj/$(platform_pathname)/$(mode_pathname)$(suffix)
libs_dir = $(path_step)/lib/$(platform_pathname)/$(mode_pathname)$(suffix)
bins_dir = $(path_step)/bin/$(platform_pathname)/$(mode_pathname)

#set platform-specific parameters
include $(path_step)/make/platforms/$(platform).mak

#set host-specific parameters
include $(path_step)/make/hosts/$(host).mak

#set environment
include $(path_step)/make/environment.mak

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
include $(path_step)/make/textator.mak
endif

#process qt if required
ifdef use_qt
include $(path_step)/make/qt.mak
endif

#process boost
include $(path_step)/make/boost.mak

#process l10n files
include $(path_step)/make/l10n.mak

GENERATED_HEADERS = $(addsuffix .h,$(generated_headers))

SOURCES = $(addsuffix $(src_suffix),$(source_files)) $(addsuffix $(src_suffix_cc),$(source_files_cc))
GENERATED_SOURCES = $(addsuffix $(src_suffix),$(generated_sources))

#calculate object files from sources
OBJECTS = $(foreach src, $(notdir $(source_files) $(source_files_cc) $(generated_sources)), $(objects_dir)/$(call makeobj_name,$(src)))

#calculate object files from windows resources
RESOURCES += $(foreach res,$(notdir $($(platform)_resources)), $(objects_dir)/$(call makeres_name,$(res)))

TRANSLATIONS = $(mo_files) $(qm_files)

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
$(target): $(OBJECTS) $(RESOURCES) | $(output_dir) $(TRANSLATIONS)
	$(call build_lib_cmd,$^,$@)

.PHONY: deps
else
#binary and dynamic libraries with dependencies
LIBS = $(foreach lib,$(libraries),$(libs_dir)/$(call makelib_name,$(lib)))

$(target): $(OBJECTS) $(RESOURCES) $(LIBS) $(embedded_files) | $(output_dir) $(TRANSLATIONS)
	$(link_cmd)
	$(postlink_cmd)
	$(if $(embedded_files),$(embed_file_cmd),)

$(LIBS): deps

deps: $(depends) $($(platform)_depends)

$(depends) $($(platform)_depends):
	$(MAKE) pic=$(pic) -C $(addprefix $(path_step)/,$@) $(MAKECMDGOALS)
endif

$(OBJECTS): | $(GENERATED_HEADERS) $(objects_dir)

$(RESOURCES): | $(objects_dir)

vpath %$(src_suffix) $(sort $(dir $(source_files) $(generated_source_files)))
vpath %$(src_suffix_cc) $(sort $(dir $(source_files_cc)))
vpath %$(res_suffix) $(sort $(dir $($(platform)_resources)))

$(objects_dir)/%$(call makeobj_name,): %$(src_suffix)
	$(call build_obj_cmd,$(CURDIR)/$<,$@)

$(objects_dir)/%$(call makeobj_name,): %$(src_suffix_cc)
	$(call build_obj_cmd_cc,$(CURDIR)/$<,$@)

$(objects_dir)/%$(call makeres_name,): %$(res_suffix)
	$(call makeres_cmd,$<,$@)

.PHONY: clean

clean: clean_self clean_deps

clean_self:
	-$(call rmfiles_cmd,$(wildcard $(target)*))
	-$(call rmdir_cmd,$(objects_dir))

clean_deps: $(depends)

install: install_$(platform)

include $(path_step)/make/codeblocks.mak

test: $(target)
ifdef binary_name
	$^
endif
