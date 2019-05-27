ifdef jumbo.enabled
ifdef tools.python
jumbo.source = $(objects_dir)/$(jumbo.name).jumbo$(suffix.cpp)
jumbo.candidates := $(source_files)
jumbo.inputs = $(filter-out $(jumbo.excludes),$(jumbo.candidates))
jumbo.ignored = $(filter $(jumbo.excludes),$(jumbo.candidates))

$(jumbo.source): $(jumbo.inputs) | $(objects_dir)
	$(tools.python) $(dirs.root)/make/tools/combine_sources.py $(foreach src,$^,$(CURDIR)/$(src)) > $@

source_files = $(jumbo.source) $(jumbo.ignored)
else
$(error tools.python required to be set in order to use jumbo build)
endif
endif
