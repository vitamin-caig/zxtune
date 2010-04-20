binary_name := zxtune-qt
path_step := ../..
include $(path_step)/default.mak
source_files := qt_app 

#ui-related
ui_files := ui/mainwindow ui/playback_controls ui/seek_controls
source_files += $(ui_files)

libraries := zxtune_app_base

depends := apps/base

$(platform)_libraries := QtGui

h_generated := $(ui_files:=_ui.h) $(ui_files:=_moc.h)

include $(path_step)/makefile.mak

UIC := uic
MOC := moc

%_ui.h: %.ui
	$(UIC) $< -o $@

%_moc.h: %.h
	$(MOC) -i $< -o $@
