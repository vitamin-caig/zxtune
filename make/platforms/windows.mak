makebin_name = $(1).exe
makelib_name = $(1).lib
makedyn_name = $(1).dll
makeobj_name = $(1).obj

compiler := $(if $(compiler),$(compiler),msvs)

windows_libraries += $(foreach lib,$(boost_libraries),boost_$(lib)-$(MSVS_VERSION)-mt$(if $(mode) eq debug,-gd,)-$(BOOST_VERSION).lib)
