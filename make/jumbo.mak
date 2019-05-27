ifdef jumbo.enabled
ifdef tools.python
jumbo_source = $(objects_dir)/$(jumbo.name).jumbo$(suffix.cpp)

$(jumbo_source): $(source_files) | $(objects_dir)
	$(tools.python) $(dirs.root)/make/tools/combine_sources.py $(foreach src,$^,$(CURDIR)/$(src)) > $@

source_files = $(jumbo_source)
else
$(error tools.python required to be set in order to use jumbo build)
endif
endif
