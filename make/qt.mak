suffix.ui = .ui
suffix.ui.h = .ui.h
suffix.moc.cpp = .moc$(suffix.cpp)
suffix.qrc = .qrc
suffix.qrc.cpp = $(suffix.qrc)$(suffix.cpp)

generated_dir = $(objects_dir)/.qt

generated_headers += $(addprefix $(generated_dir)/,$(notdir $(ui_files:=$(suffix.ui.h))))
generated_sources += $(addprefix $(generated_dir)/,$(notdir $(ui_files:=$(suffix.moc.cpp)) $(moc_files:=$(suffix.moc.cpp)) $(qrc_files:=$(suffix.qrc.cpp))))

include_dirs += $(generated_dir)

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

vpath %$(suffix.qrc) $(sort $(dir $(qrc_files)))
vpath %$(suffix.ui) $(sort $(dir $(ui_files)))
vpath %.h $(sort $(dir $(moc_files) $(ui_files) $(generated_dir)))

$(generated_dir):
	$(call makedir_cmd,$@)

tools.uic ?= $(qt.bin)uic
tools.moc ?= $(qt.bin)moc
tools.rcc ?= $(qt.bin)rcc

$(generated_dir)/%$(suffix.ui.h): %$(suffix.ui) | $(generated_dir)
	$(tools.uic) $< -o $@

$(generated_dir)/%$(suffix.moc.cpp): %.h | $(generated_dir)
	$(tools.moc) -nw $(addprefix -D,$(DEFINITIONS)) $< -o $@

$(generated_dir)/%$(suffix.moc.cpp): %$(suffix.ui.h) | $(generated_dir)
	$(tools.moc) -nw $(addprefix -D,$(DEFINITIONS)) $< -o $@

$(generated_dir)/%$(suffix.qrc.cpp): %$(suffix.qrc) | $(generated_dir)
	$(tools.rcc) -name $(basename $(notdir $<)) -o $@ $<

#disable dependencies generation for generated files
$(objects_dir)/$(call makeobj_name,%$(suffix.moc.cpp)): $(generated_dir)/%$(suffix.moc.cpp)
	$(call build_obj_cmd_nodeps,$(CURDIR)/$<,$@)

$(objects_dir)/$(call makeobj_name,%$(suffix.qrc.cpp)): $(generated_dir)/%$(suffix.qrc.cpp)
	$(call build_obj_cmd_nodeps,$(CURDIR)/$<,$@)
