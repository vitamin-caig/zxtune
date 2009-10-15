makebin_name = $(1).exe
makelib_name = $(1).lib
makedyn_name = $(1).dll
makeobj_name = $(1).obj

#add BOOST include directory
include_dirs := $(include_dirs) C:/Boost/include/boost-1_39

compiler := $(if $(compiler),$(compiler),msvs)
