makebin_name = $(1).exe
makelib_name = lib$(1).a
makedyn_name = $(1).dll
makeobj_name = $(1).o
makedir_cmd = if NOT EXIST $(subst /,\,$(1)) mkdir $(subst /,\,$(1))

compiler=gcc
cxx_options += -march=native

#built-in features
support_waveout = 1

#simple library naming convention used
mingw_libraries += $(foreach lib,$(boost_libraries),boost_$(lib))
