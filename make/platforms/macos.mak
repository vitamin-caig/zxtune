makebin_name = $(1)
makelib_name = lib$(1).a
makedyn_name = lib$(1).so
makeobj_name = $(1).o
makedir_cmd = mkdir -p $(1)
rmdir_cmd = rm -Rf $(1)
rmfiles_cmd = rm -f $(1)
showtime_cmd = date +"%x %X"

compiler=gcc
LINKER_BEGIN_GROUP=
LINKER_END_GROUP=

macos_libraries += $(foreach lib,$(libraries.boost),boost_$(lib))

CXX_PLATFORM_FLAGS += $(addprefix -FQt,$(libraries.qt))
LD_PLATFORM_FLAGS += $(addprefix -framework Qt,$(libraries.qt))
