suffix.txt = .txt

dep_texts = $(other_text_files:=$(suffix.txt))
generated_headers += $(text_files:=.h)
generated_sources += $(text_files:=$(suffix.cpp))

ifdef tools.textator
texts_dir := $(dir $(text_files))

#textator can be get from
#http://code.google.com/p/textator
#if were no changes in txt files, just touch .h and .cpp files in this folder or change TEXTATOR to true
#by default, disable it
tools.textator.flags = --verbose --process --cpp --symboltype "Char" --memtype "extern const" --tab 2 --width 112

#$(srcs_dir)/%$(txt_suffix)$(src_suffix): %$(txt_suffix) $(dep_texts) | $(srcs_dir)
%$(suffix.cpp): %$(suffix.txt) $(dep_texts)
	$(tools.textator) $(tools.textator.flags) --inline --output $@ $<

%.h: %$(suffix.txt) $(dep_texts)
	$(tools.textator) $(tools.textator.flags) --noinline --output $@ $<
endif
