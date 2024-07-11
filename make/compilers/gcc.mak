#basic definitions for tools
tools.cxx = $(tools.cxxwrapper) $($(platform).$(arch).execprefix)g++
tools.cc = $(tools.ccwrapper) $($(platform).$(arch).execprefix)gcc
tools.ld ?= $($(platform).$(arch).execprefix)g++
tools.ar ?= $($(platform).$(arch).execprefix)ar
tools.objcopy ?= $($(platform).$(arch).execprefix)objcopy
tools.strip ?= $($(platform).$(arch).execprefix)strip
tools.nm ?= $($(platform).$(arch).execprefix)nm
tools.objdump ?= $($(platform).$(arch).execprefix)objdump

LINKER_BEGIN_GROUP ?= -Wl,'-('
LINKER_END_GROUP ?= -Wl,'-)'

#set options according to mode
ifdef release
CXX_MODE_FLAGS = -O2 -DNDEBUG -funroll-loops -finline-functions
else
CXX_MODE_FLAGS = -O0
endif

#setup profiling
ifdef profile
CXX_MODE_FLAGS += -pg
LD_MODE_FLAGS += -pg
else
CXX_MODE_FLAGS += -fdata-sections -ffunction-sections
ifdef release
LD_MODE_FLAGS += -Wl,-O3,--gc-sections,--relax
endif
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
INCLUDES_DIRS = $(sort $(includes.dirs) $(includes.dirs.$(platform)) $(includes.dirs.$(notdir $1)))
INCLUDES_FILES = $(includes.files) $(includes.files.$(platform))

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
build_obj_cmd_cc = $(tools.cc) $(CCFLAGS) -c $$(realpath $1) -o $2 -MMD
build_lib_cmd = $(tools.ar) $(ARFLAGS) $2 $1
link_cmd = $(tools.ld) $(LDFLAGS) -o $@ $(OBJECTS) $(RESOURCES) \
        -L$(libraries.dir) $(LINKER_BEGIN_GROUP) $(addprefix -l,$(libraries)) $(LINKER_END_GROUP) \
        $(if $(libraries.static),$(LINKER_BEGIN_GROUP) $(addprefix -l:,$(foreach l,$(libraries.static),$(call makelib_name,$(l)))) $(LINKER_END_GROUP)) \
        $(addprefix -L,$(libraries.dirs.$(platform)))\
        $(LINKER_BEGIN_GROUP) $(addprefix -l,$(sort $(libraries.$(platform)))) $(LINKER_END_GROUP)\
	$(if $(libraries.dynamic),-L$(output_dir) $(addprefix -l,$(libraries.dynamic)),)

#specify postlink command- generate pdb file
postlink_cmd = $(tools.objcopy) --only-keep-debug $@ $@.pdb && $(sleep_cmd) && \
	$(tools.objcopy) --strip-all $@ && $(sleep_cmd) && \
	$(tools.objcopy) --add-gnu-debuglink=$@.pdb $@

#include generated dependensies
include $(wildcard $(objects_dir)/*.d)

.PHONY: analyze analyze_all

analyze:
	@echo "Analyzing $(target)" > coverage.log
	@for i in $(SOURCES);do gcov -lp -o $(objects_dir) $$i >> coverage.log; done
	@echo `pwd`
	@perl $(dirs.root)/make/compilers/gcc_coverage.pl

analyze_deps: $(depends)

analyze_all: analyze analyze_deps

.PHONY: topsymbols symbolstree websymbolstree

ifdef tools.python
ifneq ($(binary_name)$(dynamic_name),)

$(target).nm.out: $(target)
	$(tools.nm) -C -S -l $^.pdb > $@

$(target).objdump.out: $(target)
	$(tools.objdump) -h $^.pdb > $@

tools_dir = $(abspath $(dirs.root)/make/tools)
bloat_dir = $(tools_dir)/bloat
call_bloat_cmd = $(tools.python) $(bloat_dir)/bloat.py --nm-output $(target).nm.out --objdump-output $(target).objdump.out --strip-prefix $(abspath $(dirs.root))/

topsymbols: $(target).nm.out $(target).objdump.out
	$(call_bloat_cmd) dump > $(target).topsymbols

$(target).syms.json: $(target).nm.out $(target).objdump.out
	$(call_bloat_cmd) syms > $@

symbolstree: $(target).syms.json

websymbolstree: $(target).syms.json
	(cd $(bloat_dir) && $(tools.python) $(tools_dir)/server.py /bloat.json=$(abspath $^))
endif
endif