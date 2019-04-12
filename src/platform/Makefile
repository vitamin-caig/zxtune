library_name := platform
dirs.root := ../..
source_dirs := src

include $(dirs.root)/make/default.mak

ifneq ($(findstring $(platform),windows mingw),)
source_dirs += src/windows
else ifneq ($(findstring $(platform),linux darwin),)
source_dirs += src/linux
else
source_dirs += src/$(platform)
endif

po_source_dirs = src src/windows src/linux
po_files := platform

include $(dirs.root)/makefile.mak
