library_name := tools
path_step := ../..
source_dirs := src

include $(path_step)/make/default.mak
ifneq ($(findstring $(platform),windows mingw),)
source_files += platform/shared_library_windows
else ifneq ($(findstring $(platform),linux dingux macos),)
source_files += platform/shared_library_linux
else
source_files += platform/shared_library_$(platform)
endif

source_files += platform/shared_library_common

text_files := text/tools
po_files := tools

include $(path_step)/makefile.mak
