makebin_name = $(1).log
makelib_name = $(1).log
makedyn_name = $(1).log
makeobj_name = $(1).log

host=linux

compiler?=iwyu

ifeq ($(compiler),iwyu)
tools.cxx = iwyu $(addprefix -Xiwyu ,--mapping_file=$(dirs.root)/make/tools/iwyu/iwyu_mapping.imp --cxx17ns)
tools.cc = true
tools.ar = true
tools.ld = true
tools.objcopy = true
tools.strip = true
endif

compiler=clang
