txt_suffix = .txt

dep_texts = $(other_text_files:=$(txt_suffix))
generated_headers += $(text_files)
#generated_sources += $(notdir $(text_files:=$(txt_suffix)))
source_files += $(text_files)

#textator can be get from
#http://code.google.com/p/textator
#if were no changes in txt files, just touch .h and .cpp files in this folder or change TEXTATOR to true
ifdef USE_TEXTATOR
TEXTATOR := textator
TEXTATOR_FLAGS := --verbose --process --cpp --symboltype "Char" --memtype "extern const" --tab 2 --width 112
else
TEXTATOR := true
endif

#$(srcs_dir)/%$(txt_suffix)$(src_suffix): %$(txt_suffix) $(dep_texts) | $(srcs_dir)
%$(src_suffix): %$(txt_suffix) $(dep_texts)
	$(TEXTATOR) $(TEXTATOR_FLAGS) --inline --output $@ $<

%.h: %$(txt_suffix) $(dep_texts)
	$(TEXTATOR) $(TEXTATOR_FLAGS) --noinline --output $@ $<
