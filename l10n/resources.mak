l10n_languages := ru

l10n_dir = $(dirs.root)/l10n

ifeq ($(host),windows)
fix_path_cmd = $(subst /,\,$(1))
else
fix_path_cmd = $(1)
endif

mo_translation_data := $(l10n_dir)/gettext.zip

mo_translation_files = $(wildcard $(foreach lang,$(l10n_languages),$(l10n_dir)/$(lang)/*.mo))

$(mo_translation_data): $(mo_translation_files)
	(cd $(call fix_path_cmd,$(l10n_dir)) && $(tools.root)zip -v9uD $(notdir $@) $(call fix_path_cmd,$(subst $(l10n_dir)/,,$^)))

vpath %.po $(addprefix $(l10n_dir)/,$(l10n_languages))))

qm_translation_data := $(l10n_dir)/qt.zip

qm_translation_files = $(wildcard $(foreach lang,$(l10n_languages),$(l10n_dir)/$(lang)/*.qm))

#order-only dependency
$(qm_translation_data): $(qm_translation_files) | $(mo_translation_data)
	(cd $(call fix_path_cmd,$(l10n_dir)) && $(tools.root)zip -v9uD $(notdir $@) $(call fix_path_cmd,$(subst $(l10n_dir)/,,$^)))

vpath %.ts $(addprefix $(l10n_dir)/,$(l10n_languages))))
