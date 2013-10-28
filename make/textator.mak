txt_suffix = .txt

dep_texts = $(other_text_files:=$(txt_suffix))
generated_headers += $(text_files)
#generated_sources += $(notdir $(text_files:=$(txt_suffix)))
source_files += $(text_files)

ifdef tools.textator
#textator can be get from
#http://code.google.com/p/textator
#if were no changes in txt files, just touch .h and .cpp files in this folder or change TEXTATOR to true
#by default, disable it
tools.textator.flags = --verbose --process --cpp --symboltype "Char" --memtype "extern const" --tab 2 --width 112

#$(srcs_dir)/%$(txt_suffix)$(src_suffix): %$(txt_suffix) $(dep_texts) | $(srcs_dir)
%$(src_suffix): %$(txt_suffix) $(dep_texts)
	$(tools.textator) $(tools.textator.flags) --inline --output $@ $<

%.h: %$(txt_suffix) $(dep_texts)
	$(tools.textator) $(tools.textator.flags) --noinline --output $@ $<
endif
