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
includes.dirs += $(dirs.root)/include $(dirs.root)/src $(dirs.root)
objs_dir = $(dirs.root)/obj/$(platform_pathname)/$(mode_pathname)$(submode_pathname)
libraries.dir = $(dirs.root)/lib/$(platform_pathname)/$(mode_pathname)$(submode_pathname)
bins_dir ?= $(dirs.root)/bin/$(platform_pathname)/$(mode_pathname)

#set environment
include $(dirs.root)/make/environment.mak

#set platform-specific parameters
include $(dirs.root)/make/platforms/$(platform).mak

#set host-specific parameters
include $(dirs.root)/make/hosts/$(host).mak

#set features
include $(dirs.root)/make/features.mak

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

# Support using the most generic prefixes with clarification for particular conditions:
# platform.arch.execprefix > platform.execprefix > execprefix
$(platform).execprefix ?= $(execprefix)
$(platform).$(arch).execprefix ?= $($(platform).execprefix)

#set compiler-specific parameters
include $(dirs.root)/make/compilers/$(compiler).mak

#calculate input source files
source_files += $(source_files.$(platform))
source_dirs += $(source_dirs.$(platform))
ifdef source_dirs
source_files += $(foreach suffix,$(suffix.src),$(foreach dir,$(source_dirs),$(wildcard $(dir)/*$(suffix))))
endif

#process qt if required
ifdef use_qt
include $(dirs.root)/make/qt.mak
endif

#process boost
include $(dirs.root)/make/boost.mak

#process l10n files
include $(dirs.root)/make/l10n.mak

ifdef jumbo.name
include $(dirs.root)/make/jumbo.mak
endif

ifdef objects_flat_names
SRC2OBJ = $(objects_dir)/$(call makeobj_name,$(subst /,_,$(1)))
else
SRC2OBJ = $(objects_dir)/$(call makeobj_name,$(notdir $(1)))
endif

#calculate object files from sources
SOURCES = $(source_files) $(generated_sources)
OBJECTS = $(foreach src,$(SOURCES),$(call SRC2OBJ,$(src)))
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
include $(dirs.root)/libraries.mak

#binary and dynamic libraries with dependencies
LIBS = $(foreach lib,$(libraries),$(libraries.dir)/$(call makelib_name,$(lib)))

$(target): $(OBJECTS) $(LIBS) $(embedded_files) | $(output_dir) $(TRANSLATIONS)
	$(link_cmd)
	$(postlink_cmd)
	$(if $(embedded_files),$(embed_file_cmd),)

$(LIBS): deps

deps: $(depends) $($(platform)_depends)

$(depends) $($(platform)_depends):
	$(MAKE) pic=$(pic) static_runtime=$(static_runtime) -C $(addprefix $(dirs.root)/,$@) $(MAKECMDGOALS)
endif

$(OBJECTS): $(generated_headers) $(generated_sources) | $(objects_dir)

define COMPILE
$(SRC2OBJ): $(1)
	$(call $(2),$$(CURDIR)/$(1),$$@)
endef

$(foreach CPP,$(filter $(addprefix %,$(suffix.cpp.all)),$(SOURCES)),$(eval $(call COMPILE,$(CPP),build_obj_cmd)))
$(foreach C,$(filter %$(suffix.c),$(SOURCES)),$(eval $(call COMPILE,$(C),build_obj_cmd_cc)))
$(foreach RES,$(filter %$(suffix.res),$(SOURCES)),$(eval $(call COMPILE,$(RES),makeres_cmd)))

.PHONY: clean

clean: clean_self clean_deps

clean_self:
	-$(call rmfiles_cmd,$(wildcard $(target)*))
	-$(call rmdir_cmd,$(objects_dir))

clean_deps: $(depends)

install: install_$(platform)

include $(dirs.root)/make/codeblocks.mak

test: $(target)
ifdef binary_name
	$^
endif

report:
	$(info src=$(source_files))
	$(info src.gen=$(generated_sources))
	$(info hdr.gen=$(generated_headers))
	$(info obj=$(OBJECTS))
