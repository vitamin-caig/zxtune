makebin_name = $(1).exe
makelib_name = lib$(1).a
makedyn_name = $(1).dll
makeobj_name = $(1).o

arch=i686
compiler=gcc
compiler_version=34

#for supporting windows waveout
support_waveout = 1
mingw_include_dirs += /usr/include/w32api
mingw_libraries_dirs += /lib/w32api

mingw_libraries += $(foreach lib,$(boost_libraries),boost_$(lib)-$(compiler)$(compiler_version)-mt)
