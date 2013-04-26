makebin_name = $(1).dge
makelib_name = lib$(1).a
makedyn_name = lib$(1).so
makeobj_name = $(1).o

host=linux
compiler=gcc

#defines without spaces
dingux_libraries += stdc++ gcc c m dl pthread

#support only static multithread release libraries
dingux_libraries += $(foreach lib,$(libraries.boost),boost_$(lib))

#support static release libraries
dingux_libraries += $(foreach lib,$(libraries.qt),Qt$(lib))
