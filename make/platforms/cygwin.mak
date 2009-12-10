makebin_name = $(1).exe
makelib_name = lib$(1).a
makedyn_name = $(1).dll
makeobj_name = $(1).o

arch=i686
compiler=gcc
compiler_version=34

cygwin_libraries += $(foreach lib,$(boost_libraries),boost_$(lib)-$(compiler)$(compiler_version)-mt)
