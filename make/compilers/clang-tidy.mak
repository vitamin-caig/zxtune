#basic definitions for tools
tools.clang-tidy ?= $($(real_platform).execprefix)clang-tidy

#set options according to mode
ifdef release
CXX_MODE_FLAGS = -O2 -DNDEBUG -funroll-loops
else
CXX_MODE_FLAGS = -O0
endif

#setup PIC code
ifdef pic
CXX_MODE_FLAGS += -fPIC
endif

DEFINES = $(defines) $(defines.$(real_platform)) $(defines.$(real_platform).$(real_arch))
INCLUDES_DIRS = $(foreach i,$(sort $(includes.dirs) $(includes.dirs.$(real_platform)) $(includes.dirs.$(notdir $1))),$(realpath $(i)))
INCLUDES_FILES = $(includes.files) $(includes.files.$(real_platform))

#setup flags
CCFLAGS = -g $(CXX_MODE_FLAGS) $(cxx_flags) $($(real_platform).cxx.flags) $($(real_platform).$(real_arch).cxx.flags) \
	$(addprefix -D,$(DEFINES)) \
	-funsigned-char -fno-strict-aliasing -fvisibility=hidden \
	-W -Wall -Wextra -pipe \
	$(addprefix -I,$(INCLUDES_DIRS)) $(addprefix -include ,$(INCLUDES_FILES))

CXXFLAGS = $(CCFLAGS) -std=c++17 -fvisibility-inlines-hidden

#specify endpoint commands
build_obj_cmd_nodeps = $(tools.clang-tidy) $(clang-tidy_flags) '-header-filter=$(abspath $(dirs.root))/(src|include|apps)' $$(realpath $1) -- $(CXXFLAGS) > $2 || mv $2 $2.fail
build_obj_cmd = $(build_obj_cmd_nodeps)
build_obj_cmd_cc = true
build_lib_cmd = cat $1 > $2 || $(call rmfiles_cmd,$2)
link_cmd = cat $(OBJECTS) > $@ || $(call rmfiles_cmd,$@)
postlink_cmd = true
