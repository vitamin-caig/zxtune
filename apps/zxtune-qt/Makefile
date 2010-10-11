binary_name := zxtune-qt
path_step := ../..
include $(path_step)/default.mak
source_files := qt_app \
                supp/playitems_provider \
                ui/format
#ui-related
include_path = $(path_step)/apps/$(binary_name)
ifeq ($(platform),dingux)
ui_files := ui/mainwindow_embedded
else
ui_files := ui/mainwindow
endif

ui_files += ui/aboutdialog \
            ui/controls/playback_controls \
            ui/controls/seek_controls \
            ui/controls/status_control \
            ui/controls/volume_control \
            ui/playlist/playlist

moc_files := ui/controls/analyzer_control \
             ui/playlist/playlist_thread \
             supp/playback_supp

source_files += $(ui_files) $(moc_files) 
generated_files += $(ui_files:=_ui.h) $(ui_files:=_moc.h) $(moc_files:=_moc.h)

text_files := text/text

libraries := zxtune_app_base core devices io sound tools
depends := apps/base src/core src/devices src/io src/sound src/tools

boost_libraries := thread

qt_libraries := Gui Core
windows_libraries := gdi32 user32 advapi32 shell32 ole32 oleaut32 imm32 comdlg32 wsock32 winspool uuid oldnames $(if $(subst release,,$(mode)),comsuppwd,comsuppw)
mingw_libraries := gdi32 shell32 ole32 imm32 comdlg32 wsock32 winspool uuid
qt_defines = QT_LARGEFILE_SUPPORT QT_GUI_LIB QT_CORE_LIB QT_THREAD_SUPPORT

definitions = $(qt_defines)

#packaging
pkg_additional_files_linux := dist/linux/zxTune.desktop res/zxtune.png
pkg_additional_files_dingux := dist/dingux/menuitem.cfg dist/dingux/zxtune-wrapper.sh dist/dingux/readme.txt res/zxtune.png

qrc_files := res/icons
source_files += $(qrc_files)

#platform-specific resources
resource_files := res/win32
object_files += $(foreach rc,$(resource_files),$(objects_dir)/$(call makeres_name,$(notdir $(rc))))

include $(path_step)/makefile.mak

VPATH += $(dir $(qrc_files) $(resource_files))

UIC := uic
MOC := moc
RCC := rcc

%_ui.h: %.ui
	$(UIC) $< -o $@

%_moc.h: %.h
	$(MOC) $(addprefix -D, $(qt_defines)) -i $< -o $@

%.cpp: %.qrc
	$(RCC) -name $(basename $(notdir $<)) -o $@ $<

$(objects_dir)/%$(call makeres_name,): %.rc | $(objects_dir)
	$(call makeres_cmd,$<,$@)
