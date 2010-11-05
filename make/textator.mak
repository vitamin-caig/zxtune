dep_texts = $(other_text_files:=.txt)
source_files += $(text_files)
generated_files += $(text_files:=.h)

#textator can be get from
#http://code.google.com/p/textator
#if were no changes in txt files, just touch .h and .cpp files in this folder or change TEXTATOR to true
TEXTATOR := textator
TEXTATOR_FLAGS := --verbose --process --cpp --symboltype "Char" --memtype "extern const" --tab 2 --width 112

%$(src_suffix): %.txt $(dep_texts)
	$(TEXTATOR) $(TEXTATOR_FLAGS) --inline --output $@ $<

%.h: %.txt $(dep_texts)
	$(TEXTATOR) $(TEXTATOR_FLAGS) --noinline --output $@ $<
