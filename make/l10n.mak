include $(dirs.root)/l10n/resources.mak

ifdef po_files
include $(dirs.root)/make/l10n_po.mak
endif

ifdef ts_files
include $(dirs.root)/make/l10n_ts.mak
endif
