makebin_name = $(1)
makelib_name = lib$(1).a
makedyn_name = lib$(1).so
makeobj_name = $(1).o
makedir_cmd = mkdir -p $(1)
rmdir_cmd = rm -Rf $(1)
rmfiles_cmd = rm -f $(1)
showtime_cmd = date +"%x %X"

compiler=gcc
CXX_PLATFORM_FLAGS = -fvisibility=hidden -fvisibility-inlines-hidden
LINKER_BEGIN_GROUP=
LINKER_END_GROUP=

#built-in features
support_sdl = 1

macos_libraries += $(foreach lib,$(boost_libraries),boost_$(lib))

CXX_PLATFORM_FLAGS += $(addprefix -FQt,$(qt_libraries))
LD_PLATFORM_FLAGS += $(addprefix -framework Qt,$(qt_libraries))
