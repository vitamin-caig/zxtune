makebin_name = $(1)
makelib_name = lib$(1).a
makedyn_name = lib$(1).so
makeobj_name = $(1).o

compiler=gcc

#built-in features
support_oss = 1

linux_libraries += $(foreach lib,$(boost_libraries),boost_$(lib)-mt)
