include $(path_step)/l10n/resources.mak

ifdef po_files
include $(path_step)/make/l10n_po.mak
endif

ifdef ts_files
include $(path_step)/make/l10n_ts.mak
endif
