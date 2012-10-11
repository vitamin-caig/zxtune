po_dir = $(objects_dir)/.po

#path/lang/domain.po
getlang = $(lastword $(subst /, ,$(dir $(1))))

%.po: $(po_dir)/messages.pot
		$(if $(wildcard $@),msgmerge --update --lang=$(call getlang,$@) $@ $^,msginit --locale=$(call getlang,$@) --input $^ --output $@)

.PRECIOUS: %.po

ifdef po_source_dirs
po_source_files = $(addsuffix /*$(src_suffix),$(po_source_dirs))
else
po_source_files = $(addsuffix *$(src_suffix),$(sort $(dir $(source_files))))
endif

$(po_dir)/messages.pot: $(sort $(wildcard $(po_source_files))) | $(po_dir)
		xgettext --c++ --escape --boost --from-code=UTF-8 --omit-header \
		  --keyword=translate:1,1t --keyword=translate:1,2,3t \
		  --keyword=translate:1c,2,2t --keyword=translate:1c,2,3,4t \
		  --output $@ $^

$(po_dir):
	$(call makedir_cmd,$@)

mo_files = $(foreach lng,$(l10n_languages),$(foreach file,$(po_files),$(l10n_dir)/$(lng)/$(file).mo ))
