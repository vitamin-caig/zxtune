library_name := platform
path_step := ../..
source_dirs := src

include $(path_step)/make/default.mak

ifneq ($(findstring $(platform),windows mingw),)
source_dirs += src/windows
else ifneq ($(findstring $(platform),linux dingux macos),)
source_dirs += src/linux
else
source_dirs += src/$(platform)
endif

po_files := ru/platform

include $(path_step)/makefile.mak
