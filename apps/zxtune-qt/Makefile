binary_name := zxtune-qt
path_step := ../..
include $(path_step)/default.mak
source_files := qt_app supp/playitems_provider ui/format

#ui-related
include_path = $(path_step)/apps/$(binary_name)
ifeq ($(platform),dingux)
ui_files := ui/mainwindow_embedded
else
ui_files := ui/mainwindow
endif

ui_files += ui/controls/playback_controls ui/controls/seek_controls ui/controls/status_control ui/playlist/playlist
moc_files := ui/controls/analyzer_control ui/playlist/playlist_thread supp/playback_supp
qrc_files := res/icons
source_files += $(ui_files) $(moc_files) $(qrc_files:=_res)
text_files := text/text

libraries := zxtune_app_base core io sound tools
depends := apps/base src/core src/io src/sound src/tools

boost_libraries := thread

qt_libraries := Gui Core
windows_libraries := gdi32 user32 advapi32 shell32 ole32 oleaut32 imm32 comdlg32 wsock32 winspool uuid oldnames $(if $(subst release,,$(mode)),comsuppwd,comsuppw)
qt_defines = QT_LARGEFILE_SUPPORT QT_GUI_LIB QT_CORE_LIB QT_THREAD_SUPPORT

definitions = $(qt_defines)
generated_files += $(ui_files:=_ui.h) $(ui_files:=_moc.h) $(moc_files:=_moc.h) $(qrc_files:=_res.cpp)

include $(path_step)/makefile.mak

UIC := uic
MOC := moc
RCC := rcc

%_ui.h: %.ui
	$(UIC) $< -o $@

%_moc.h: %.h
	$(MOC) $(addprefix -D, $(qt_defines)) -i $< -o $@

%_res.cpp: %.qrc res/*.png
	$(RCC) -name $(basename $(notdir $<)) -o $@ $<
