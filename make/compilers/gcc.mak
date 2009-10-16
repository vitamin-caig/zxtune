arch := $(if $(arch),$(arch),native)
CXX := $(if $(CXX),$(CXX),g++)
LDD := $(if $(LDD),$(LDD),g++)
AR := $(if $(AR),$(AR),ar)

#set options according to mode
ifeq ($(mode),release)
cxx_mode_flags := -O3 -DNDEBUG
else ifeq ($(mode),debug)
cxx_mode_flags := -O0
else
$(error Invalid mode)
endif

ifdef profile
cxx_mode_flags := $(cxx_mode_flags) -pg
ld_mode_flags := $(ld_mode_flags) -pg
endif

ifdef pic
cxx_mode_flags := $(cxx_mode_flags) -fPIC
ld_mode_flags := $(ld_mode_flags) -shared
endif

CXX_FLAGS := $(cxx_mode_flags) $(cxx_flags) -g3 \
	$(addprefix -D, $(definitions)) \
	-march=$(arch) \
	-funroll-loops -funsigned-char -fno-strict-aliasing \
	-W -Wall -ansi -pipe \
	$(addprefix -I, $(include_dirs))

AR_FLAGS := cru
LD_FLAGS := $(ld_mode_flags) $(ld_flags) -pipe

build_obj_cmd = $(CXX) $(CXX_FLAGS) -c -MD $< -o $@
build_lib_cmd = $(AR) $(AR_FLAGS) $@ $^
link_cmd = $(LDD) $(LD_FLAGS) -o $@ $(object_files) \
	-L$(libs_dir) $(addprefix -l,$(libraries)) \
	-L$(output_dir) $(addprefix -l,$(dynamic_libs))

postlink_cmd = objcopy --only-keep-debug $@ $@.pdb && \
	strip $@ && \
	objcopy --add-gnu-debuglink=$@.pdb $@


include $(wildcard $(objects_dir)/*.d)
