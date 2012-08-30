l10n: $(if deps,deps,) $(addsuffix .mo,$(addprefix $(path_step)/l10n/,$(po_files)))

l10n_source: $(if deps,deps,) $(addsuffix .po,$(addprefix $(path_step)/l10n/,$(po_files)))

l10n_archive = $(path_step)/l10n/data.zip

po_dir = $(objects_dir)/.po

vpath %.po $(path_step)/l10n

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
		
$(po_dir) $(mo_dir):
	$(call makedir_cmd,$@)

$(l10n_archive): l10n
	(cd $(subst /,\,$(path_step)/l10n) && zip -9DR $(CURDIR)\$(subst /,\,$(@)) "*.mo")

	