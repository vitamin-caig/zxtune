binary_name := zxtune-qt
path_step := ../..
include $(path_step)/default.mak
source_files := qt_app 

#ui-related
ui_files := ui/mainwindow ui/playback_controls ui/seek_controls ui/playlist
moc_files := ui/playlist_thread
source_files += $(ui_files) $(moc_files)

libraries := zxtune_app_base core io tools
depends := apps/base src/core src/io src/tools

boost_libraries := thread

qt_libraries := Gui Core
windows_libraries := gdi32 user32 advapi32 shell32 ole32 oleaut32 imm32 comdlg32 wsock32 winspool uuid oldnames $(if $(subst release,,$(mode)),comsuppwd,comsuppw)

h_generated := $(ui_files:=_ui.h) $(ui_files:=_moc.h) $(moc_files:=_moc.h)

include $(path_step)/makefile.mak

UIC := uic
MOC := moc

%_ui.h: %.ui
	$(UIC) $< -o $@

%_moc.h: %.h
	$(MOC) -i $< -o $@
