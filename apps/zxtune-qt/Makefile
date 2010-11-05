binary_name := zxtune-qt
path_step := ../..
include $(path_step)/default.mak
source_files = qt_app \
               supp/playitems_provider \
               ui/format

#playlist
moc_files += supp/playlist/playlist_scanner supp/playlist/playlist_model supp/playlist/playlist supp/playlist/playlist_container

#ui-related

#main window is specific for platforms
ifeq ($(platform),dingux)
ui_files += ui/mainwindow_embedded
else
ui_files += ui/mainwindow
endif

#controls
ui_files += ui/controls/playback_controls \
            ui/controls/seek_controls \
            ui/controls/status_control \
            ui/controls/volume_control

moc_files += ui/controls/analyzer_control

#informational
ui_files += ui/informational/aboutdialog \
            ui/informational/componentsdialog

#moc_files += ui/informational/components_model

#playlist ui
ui_files += ui/playlist/playlist_container_view ui/playlist/playlist_scanner_view
moc_files +=  ui/playlist/playlist_view ui/playlist/playlist_table_view

#other
moc_files += supp/playback_supp

ui_dir := .ui
moc_dir := .moc

ui_headers := $(addprefix $(ui_dir)/,$(notdir $(ui_files:=_ui.h)))
moc_headers := $(addprefix $(moc_dir)/,$(notdir $(ui_files:=_moc.h) $(moc_files:=_moc.h)))

source_files += $(ui_files) $(moc_files)
generated_files += $(ui_headers) $(moc_headers)

include_path = . $(ui_dir) $(moc_dir)

text_files := text/text

libraries := zxtune_app_base core devices io sound tools
depends := apps/base src/core src/devices src/io src/sound src/tools

boost_libraries := thread

qt_libraries := Gui Core
windows_libraries := gdi32 user32 advapi32 shell32 ole32 oleaut32 imm32 comdlg32 wsock32 winspool uuid oldnames $(if $(subst release,,$(mode)),comsuppwd,comsuppw)
mingw_libraries := gdi32 shell32 ole32 imm32 comdlg32 wsock32 winspool uuid

definitions = QT_LARGEFILE_SUPPORT QT_GUI_LIB QT_CORE_LIB QT_THREAD_SUPPORT

#packaging
pkg_additional_files_linux := dist/linux/zxTune.desktop res/zxtune.png
pkg_additional_files_dingux := dist/dingux/menuitem.cfg dist/dingux/zxtune-wrapper.sh dist/dingux/readme.txt res/zxtune.png

#qt-based resources
qrc_files := res/icons
qrc_compiled = $(foreach qrc,$(notdir $(qrc_files)),$(objects_dir)/$(qrc).res)
generated_files += $(qrc_compiled)

#platform-specific resources
windows_resources := res/zxtune-qt
mingw_resources := res/zxtune-qt

include $(path_step)/makefile.mak
include $(path_step)/make/package.mak

object_files += $(qrc_compiled)
VPATH += $(dir $(qrc_files))

#generated headers and files
$(ui_headers): | $(ui_dir)
$(moc_headers): | $(moc_dir)
$(qrc_compiled): | $(objects_dir)

$(ui_dir) $(moc_dir):
	$(call makedir_cmd,$@)

UIC := uic
MOC := moc
RCC := rcc

$(ui_dir)/%_ui.h: %.ui | $(ui_dir)
	$(UIC) $< -o $@

$(moc_dir)/%_moc.h: %.h | $(moc_dir)
	$(MOC) $(addprefix -D, $(qt_defines)) -i $< -o $@

$(objects_dir)/%.res: %.qrc
	$(RCC) -name $(basename $(notdir $<)) -o $(objects_dir)/$(notdir $@).cpp $<
	$(call build_obj_cmd,$(objects_dir)/$(notdir $@).cpp,$@)
