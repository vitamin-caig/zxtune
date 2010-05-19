binary_name := zxtune-qt
path_step := ../..
include $(path_step)/default.mak
source_files := qt_app supp/playitems_provider

#ui-related
include_path = $(path_step)/apps/$(binary_name)
ui_files := ui/mainwindow ui/controls/playback_controls ui/controls/seek_controls ui/playlist/playlist
moc_files := ui/controls/analyzer_control ui/playlist/playlist_thread supp/playback_supp
qrc_files := res/icons
source_files += $(ui_files) $(moc_files) $(qrc_files:=_res)

libraries := zxtune_app_base core io sound tools
depends := apps/base src/core src/io src/sound src/tools

boost_libraries := thread

qt_libraries := Gui Core
windows_libraries := gdi32 user32 advapi32 shell32 ole32 oleaut32 imm32 comdlg32 wsock32 winspool uuid oldnames $(if $(subst release,,$(mode)),comsuppwd,comsuppw)

h_generated := $(ui_files:=_ui.h) $(ui_files:=_moc.h) $(moc_files:=_moc.h)
cpp_generated := $(qrc_files:=_res.cpp)

include $(path_step)/makefile.mak

UIC := uic
MOC := moc
RCC := rcc

%_ui.h: %.ui
	$(UIC) $< -o $@

%_moc.h: %.h
	$(MOC) -i $< -o $@

%_res.cpp: %.qrc res/*.png
	$(RCC) -name $(basename $(notdir $<)) -o $@ $<
