library_name := texts
path_step := ..
source_dirs := .

text_files := tools io sound core plugins

#dependent targets
source_files :=  $(text_files:=.cpp)
h_texts := $(text_files:=.h)

#textator can be get from
#http://code.google.com/p/textator
#if were no changes in txt files, just touch .h and .cpp files in this folder or change TEXTATOR to true

TEXTATOR := ~/bin/textator
TEXTATOR_FLAGS := --process --cpp --symboltype "Char" --memtype "extern const" --tab 2 --width 112

texts: $(h_texts) $(source_files)
	$(MAKE) all

%.cpp: %.txt
	$(TEXTATOR) $(TEXTATOR_FLAGS) --inline --output $@ $^

%.h: %.txt
	$(TEXTATOR) $(TEXTATOR_FLAGS) --noinline --output $@ $^

include $(path_step)/makefile.mak
