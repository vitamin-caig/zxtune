ifdef jumbo.enabled
ifdef tools.python
jumbo.source = $(objects_dir)/$(jumbo.name).jumbo$(suffix.cpp)
jumbo.candidates := $(source_files)
jumbo.inputs = $(filter-out $(jumbo.excludes),$(jumbo.candidates))
source_files = $(jumbo.source) $(filter $(jumbo.excludes),$(jumbo.candidates))

ifdef jumbo.for_generated
jumbo.generated_candidates := $(generated_sources)
jumbo.inputs += $(filter-out $(jumbo.excludes),$(jumbo.generated_candidates))
generated_sources = $(filter $(jumbo.excludes),$(jumbo.generated_candidates))
endif

$(jumbo.source): $(jumbo.inputs) | $(objects_dir)
	$(tools.python) $(dirs.root)/make/tools/combine_sources.py $(foreach src,$^,$(CURDIR)/$(src)) > $@ || $(call rmfiles_cmd,$@)
else
$(error tools.python required to be set in order to use jumbo build)
endif
endif
