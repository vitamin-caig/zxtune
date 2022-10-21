library_name := strings
dirs.root := ../..
source_dirs := src

fmtlib = $(dirs.root)/3rdparty/fmt

source_files = $(fmtlib)/src/format.cc
includes.dirs.format.cc = $(fmtlib)/include

jumbo.name := $(library_name)
jumbo.excludes := $(fmtlib)/src/format.cc

include $(dirs.root)/makefile.mak
