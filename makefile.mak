include $(path_step)/make/default.mak

.SUFFIXES:

#default suffixes
suffix.cpp := .cpp
suffix.cpp.all := $(suffix.cpp) .cxx .cc
suffix.c := .c
suffix.res := .rc

suffix.src = $(suffix.cpp.all) $(suffix.c)

#reset default rules
%$(suffix.cpp) :

%.cxx :

%.cc :

%$(suffix.c) :

%$(suffix.res) :

%.mak :

#setup modes
ifneq ($(or $(pic),$(dynamic_name)),)
pic := 1
endif

ifdef debug
mode = debug
undefine release
else
mode = release
release = 1
endif

ifneq ($(or $(libraries.qt),$(ui_files),$(moc_files),$(qrc_files)),)
use_qt := 1
endif

platform_pathname = $(platform)$(if $(arch),_$(arch),)
mode_pathname = $(mode)$(if $(profile),_profile,)
submode_pathname = $(if $(pic),_pic,)$(if $(static_runtime),_static,)

#set directories
include_dirs += $(path_step)/include $(path_step)/src $(path_step)
objs_dir = $(path_step)/obj/$(platform_pathname)/$(mode_pathname)$(submode_pathname)
libraries.dir = $(path_step)/lib/$(platform_pathname)/$(mode_pathname)$(submode_pathname)
bins_dir = $(path_step)/bin/$(platform_pathname)/$(mode_pathname)

#set environment
include $(path_step)/make/environment.mak

#set platform-specific parameters
include $(path_step)/make/platforms/$(platform).mak

#set host-specific parameters
include $(path_step)/make/hosts/$(host).mak

#set features
include $(path_step)/make/features.mak

#tune output according to type
ifdef library_name
output_dir = $(libraries.dir)
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
source_files += $(source_files.$(platform))
source_dirs += $(source_dirs.$(platform))
ifdef source_dirs
source_files += $(foreach suffix,$(suffix.src),$(foreach dir,$(source_dirs),$(wildcard $(dir)/*$(suffix))))
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

#calculate object files from sources
OBJECTS = $(foreach src,$(notdir $(source_files) $(generated_sources)), $(objects_dir)/$(call makeobj_name,$(src)))
OBJECTS.CPP = $(filter $(foreach ext,$(suffix.cpp.all),%$(call makeobj_name,$(ext))),$(OBJECTS))
OBJECTS.C = $(filter %$(call makeobj_name,$(suffix.c)),$(OBJECTS))
OBJECTS.RES = $(filter %$(call makeobj_name,$(suffix.res)),$(OBJECTS))

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
$(target): $(OBJECTS) | $(output_dir) $(TRANSLATIONS)
	$(call build_lib_cmd,$^,$@)

.PHONY: deps
else
#libraries helpers
include $(path_step)/libraries.mak

#binary and dynamic libraries with dependencies
LIBS = $(foreach lib,$(libraries),$(libraries.dir)/$(call makelib_name,$(lib)))

$(target): $(OBJECTS) $(LIBS) $(embedded_files) | $(output_dir) $(TRANSLATIONS)
	$(link_cmd)
	$(postlink_cmd)
	$(if $(embedded_files),$(embed_file_cmd),)

$(LIBS): deps

deps: $(depends) $($(platform)_depends)

$(depends) $($(platform)_depends):
	$(MAKE) pic=$(pic) static_runtime=$(static_runtime) -C $(addprefix $(path_step)/,$@) $(MAKECMDGOALS)
endif

$(OBJECTS): $(generated_headers) $(generated_sources) | $(objects_dir)

VPATH = $(sort $(dir $(source_files) $(generated_sources)))

$(OBJECTS.CPP): $(objects_dir)/$(call makeobj_name,%): %
	$(call build_obj_cmd,$(CURDIR)/$<,$@)

$(OBJECTS.C): $(objects_dir)/$(call makeobj_name,%): %
	$(call build_obj_cmd_cc,$(CURDIR)/$<,$@)

$(OBJECTS.RES): $(objects_dir)/$(call makeobj_name,%): %
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

report:
	$(info src=$(source_files))
	$(info src.gen=$(generated_sources))
	$(info hdr.gen=$(generated_headers))
	$(info obj=$(OBJECTS))
	$(info obj.cpp=$(OBJECTS.CPP))
	$(info obj.c=$(OBJECTS.C))
	$(info obj.res=$(OBJECTS.RES))
	$(info vpath=$(VPATH))
