l10n: $(if deps,deps,) $(addsuffix .po,$(po_files))

po_dir = $(objects_dir)/.po

vpath %.po $(sort $(dir $(po_files)))

#path/lang/domain.po
getlang = $(lastword $(subst /, ,$(dir $(1))))

%.po: $(po_dir)/messages.pot
		$(if $(wildcard $@),msgmerge --update --lang=$(call getlang,$@) $@ $^,msginit --locale=$(call getlang,$@) --input $^ --output $@)

$(po_dir)/messages.pot: $(SOURCES) | $(po_dir)
		xgettext --c++ --escape --boost --from-code=UTF-8 --omit-header \
		  --keyword=translate:1,1t --keyword=translate:1,2,3t \
		  --keyword=translate:1c,2,2t --keyword=translate:1c,2,3,4t \
		  --output $@ $^

%.mo: %.po
		msgfmt --output $@ $^
		
$(po_dir):
	$(call makedir_cmd,$@)
