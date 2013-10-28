ui_dir = $(objects_dir)/.ui
ui_headers = $(addprefix $(ui_dir)/,$(notdir $(ui_files:=.ui) $(ui_headeronly:=.ui)))
moc_sources = $(ui_files:=.moc) $(moc_files:=.moc)

source_files += $(ui_files) $(moc_files)
generated_headers += $(ui_headers)
generated_sources += $(moc_sources)

include_dirs += $(ui_dir)

qrc_sources = $(qrc_files:=.qrc)
generated_sources += $(qrc_sources)

ifdef release
defines += QT_NO_DEBUG
endif

ifndef distro
qt.version = $($(platform).$(arch).qt.version)
endif

ifeq ($(qt.version),)
include_dirs += $(qt.includes)
else
qt.dir = $(prebuilt.dir)/qt-$(qt.version)-$(platform)-$(arch)
include_dirs += $(qt.dir)/include
$(platform)_libraries_dirs += $(qt.dir)/lib
qt.bin = $(qt.dir)/bin/
endif

ifneq (,$(findstring Core,$(libraries.qt)))
windows_libraries += kernel32 user32 shell32 uuid ole32 advapi32 ws2_32 oldnames
mingw_libraries += kernel32 user32 shell32 uuid ole32 advapi32 ws2_32
endif
ifneq (,$(findstring Gui,$(libraries.qt)))
windows_libraries += gdi32 comdlg32 imm32 winspool ws2_32 ole32 user32 advapi32 oldnames
mingw_libraries += gdi32 comdlg32 imm32 winspool ws2_32 ole32 uuid user32 advapi32
ifneq ($($(platform).$(arch).qt.version),)
linux_libraries += freetype SM ICE Xext Xrender Xrandr Xfixes X11 fontconfig
dingux_libraries += png
endif
endif

vpath %.qrc $(sort $(dir $(qrc_files)))
vpath %.ui $(sort $(dir $(ui_files)))
vpath %.h $(sort $(dir $(moc_files) $(ui_files)))

$(ui_dir):
	$(call makedir_cmd,$@)

tools.uic ?= $(qt.bin)uic
tools.moc ?= $(qt.bin)moc
tools.rcc ?= $(qt.bin)rcc

$(ui_dir)/%.ui.h: %.ui | $(ui_dir)
	$(tools.uic) $< -o $@

%.moc$(src_suffix): %.h
	$(tools.moc) $(addprefix -D,$(DEFINITIONS)) $< -o $@

%.qrc$(src_suffix): %.qrc
	$(tools.rcc) -name $(basename $(notdir $<)) -o $@ $<

#disable dependencies generation for generated files
$(objects_dir)/%$(call makeobj_name,.moc): %.moc$(src_suffix)
	$(call build_obj_cmd_nodeps,$(CURDIR)/$<,$@)

$(objects_dir)/%$(call makeobj_name,.qrc): %.qrc$(src_suffix)
	$(call build_obj_cmd_nodeps,$(CURDIR)/$<,$@)
