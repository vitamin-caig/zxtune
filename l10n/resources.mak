l10n_languages := ru

l10n_dir = $(path_step)/l10n

ifeq ($(host),windows)
fix_path_cmd = $(subst /,\,$(1))
else
fix_path_cmd = $(1)
endif

mo_translation_data := $(l10n_dir)/gettext.zip

mo_translation_files = $(wildcard $(foreach lang,$(l10n_languages),$(l10n_dir)/$(lang)/*.mo))

$(mo_translation_data): $(mo_translation_files)
	(cd $(call fix_path_cmd,$(l10n_dir)) && zip -v9uD $(notdir $@) $(call fix_path_cmd,$(subst $(l10n_dir)/,,$^)))

%.mo: %.po
		msgfmt --output $@ $^

vpath %.po $(l10n_dir)

qm_translation_data := $(l10n_dir)/qt.zip

qm_translation_files = $(wildcard $(foreach lang,$(l10n_languages),$(l10n_dir)/$(lang)/*.qm))

$(qm_translation_data): $(qm_translation_files)
	(cd $(call fix_path_cmd,$(l10n_dir)) && zip -v9uD $(notdir $@) $(call fix_path_cmd,$(subst $(l10n_dir)/,,$^)))

%.qm: %.ts
	lrelease $^ -qm $@

vpath %.ts $(addprefix $(l10n_dir)/,$(l10n_languages))))
