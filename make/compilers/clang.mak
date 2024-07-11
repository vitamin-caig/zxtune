#basic definitions for tools
tools.cxx ?= $(tools.cxxwrapper) $($(platform).$(arch).execprefix)clang++
tools.cc ?= $(tools.ccwrapper) $($(platform).$(arch).execprefix)clang
tools.ld ?= $($(platform).$(arch).execprefix)clang++
tools.ar ?= $($(platform).$(arch).execprefix)ar
tools.objcopy ?= $($(platform).$(arch).execprefix)objcopy
tools.strip ?= $($(platform).$(arch).execprefix)strip

LINKER_BEGIN_GROUP ?= -Wl,--start-group
LINKER_END_GROUP ?= -Wl,--end-group
BUILD_ID_FLAG ?= -Wl,--build-id

#enable build id
LD_MODE_FLAGS += $(BUILD_ID_FLAG)

#set options according to mode
ifdef release
CXX_MODE_FLAGS = -O2 -DNDEBUG -funroll-loops
else
CXX_MODE_FLAGS = -O0
endif

#setup profiling
ifdef profile
CXX_MODE_FLAGS += -pg
LD_MODE_FLAGS += -pg
else
CXX_MODE_FLAGS += -fdata-sections -ffunction-sections
endif

#setup PIC code
ifdef pic
CXX_MODE_FLAGS += -fPIC
LD_MODE_FLAGS += -shared
endif

#setup code coverage
ifdef coverage
ifdef release
$(error Code coverage is not supported in release mode)
endif
CXX_MODE_FLAGS += --coverage
LD_MODE_FLAGS += --coverage
endif

DEFINES = $(defines) $(defines.$(platform)) $(defines.$(platform).$(arch))
INCLUDES_DIRS = $(foreach i,$(sort $(includes.dirs) $(includes.dirs.$(platform)) $(includes.dirs.$(notdir $1))),$(abspath $(i)))
INCLUDES_FILES = $(foreach f,$(includes.files) $(includes.files.$(platform)),$(abspath $(f)))

#setup flags
CCFLAGS = -g $(CXX_MODE_FLAGS) $(cxx_flags) $($(platform).cxx.flags) $($(platform).$(arch).cxx.flags) \
	$(addprefix -D,$(DEFINES)) \
	-funsigned-char -fno-strict-aliasing -fvisibility=hidden \
	-W -Wall -Wextra -pipe \
	$(addprefix -I,$(INCLUDES_DIRS)) $(addprefix -include ,$(INCLUDES_FILES))

CXXFLAGS = $(CCFLAGS) -std=c++20 -fvisibility-inlines-hidden

ARFLAGS := crus
LDFLAGS = $(LD_MODE_FLAGS) $($(platform).ld.flags) $($(platform).$(arch).ld.flags) $(ld_flags)

#specify endpoint commands
build_obj_cmd_nodeps = $(tools.cxx) $(CXXFLAGS) -c $$(realpath $1) -o $2
build_obj_cmd = $(build_obj_cmd_nodeps) -MMD
build_obj_cmd_cc = $(tools.cc) $(CCFLAGS) -c $$(realpath $1) -o $2
build_lib_cmd = $(tools.ar) $(ARFLAGS) $2 $1
link_cmd = $(tools.ld) $(LDFLAGS) -o $@ $(OBJECTS) $(RESOURCES) \
        -L$(libraries.dir) $(LINKER_BEGIN_GROUP) $(addprefix -l,$(libraries)) $(LINKER_END_GROUP) \
        $(if $(libraries.static),$(LINKER_BEGIN_GROUP) $(addprefix -l:,$(foreach l,$(libraries.static),$(call makelib_name,$(l)))) $(LINKER_END_GROUP)) \
        $(addprefix -L,$(libraries.dirs.$(platform)))\
        $(LINKER_BEGIN_GROUP) $(addprefix -l,$(sort $(libraries.$(platform)))) $(LINKER_END_GROUP)\
	$(if $(libraries.dynamic),-L$(output_dir) $(addprefix -l,$(libraries.dynamic)),)

#specify postlink command- generate pdb file
postlink_cmd ?= $(tools.objcopy) --only-keep-debug $@ $@.pdb && $(sleep_cmd) && \
	$(tools.objcopy) --strip-all $@ && $(sleep_cmd) && \
	$(tools.objcopy) --add-gnu-debuglink=$@.pdb $@

#include generated dependensies
include $(wildcard $(objects_dir)/*.d)
