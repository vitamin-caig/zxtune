ifdef tools.gettext
po_dir = $(objects_dir)/.po

#path/lang/domain.po
getlang = $(lastword $(subst /, ,$(dir $(1))))

%.po: $(po_dir)/messages.pot
		$(if $(wildcard $@),\
		  $(tools.gettext.root)msgmerge --update --lang=$(call getlang,$@) $@ $^,\
		  $(tools.gettext.root)msginit --locale=$(call getlang,$@) --input $^ --output $@\
		)

.PRECIOUS: %.po

ifndef po_source_dirs
po_source_dirs = $(sort $(dir $(source_files)))
endif
po_source_files = $(foreach suff,$(suffix.src),$(foreach dir,$(po_source_dirs),$(dir)/*$(suff)))

$(po_dir)/messages.pot: $(sort $(wildcard $(po_source_files))) | $(po_dir)
		$(tools.gettext.root)xgettext --c++ --escape --boost --from-code=UTF-8 --omit-header \
		  --keyword=translate:1,1t --keyword=translate:1,2,3t \
		  --keyword=translate:1c,2,2t --keyword=translate:1c,2,3,4t \
		  --output $@ $^

$(po_dir):
	$(call makedir_cmd,$@)

mo_files = $(foreach lng,$(l10n_languages),$(foreach file,$(po_files),$(l10n_dir)/$(lng)/$(file).mo ))

%.mo: %.po
		$(tools.gettext.root)msgfmt --output $@ $^
endif