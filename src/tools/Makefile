library_name := tools
path_step := ../..
source_dirs := src
ifneq ($(findstring $(platform),windows mingw),)
source_files += platform/shared_library_linux
else ifneq ($(findstring $(platform),linux macos),)
#source_files += platform/shared_library_windows
else
source_files += platform/shared_library_$(platform)
endif

text_files := text/tools

include $(path_step)/makefile.mak
