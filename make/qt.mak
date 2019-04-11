suffix.ui = .ui
suffix.ui.h = .ui.h
suffix.moc.cpp = .moc$(suffix.cpp)
suffix.qrc = .qrc
suffix.qrc.cpp = $(suffix.qrc)$(suffix.cpp)

generated_dir = $(objects_dir)/.qt

generated_headers += $(addprefix $(generated_dir)/,$(notdir $(ui_files:=$(suffix.ui.h))))
generated_sources += $(addprefix $(generated_dir)/,$(notdir $(ui_files:=$(suffix.moc.cpp)) $(moc_files:=$(suffix.moc.cpp)) $(qrc_files:=$(suffix.qrc.cpp))))

includes.dirs += $(generated_dir)

ifdef release
defines += QT_NO_DEBUG
endif

ifndef distro
qt.version = $($(platform).$(arch).qt.version)
endif

ifeq ($(qt.version),)
includes.dirs += $(qt.includes)
else
qt.dir = $(prebuilt.dir)/qt-$(qt.version)-$(platform)-$(arch)
includes.dirs += $(qt.dir)/include
libraries.dirs.$(platform) += $(qt.dir)/lib
qt.bin = $(qt.dir)/bin/
endif

ifneq (,$(findstring Core,$(libraries.qt)))
libraries.windows += kernel32 user32 shell32 uuid ole32 advapi32 ws2_32 oldnames
libraries.mingw += kernel32 user32 shell32 uuid ole32 advapi32 ws2_32
darwin.ld.flags += -framework AppKit -framework Security
endif
ifneq (,$(findstring Gui,$(libraries.qt)))
libraries.windows += gdi32 comdlg32 imm32 winspool ws2_32 ole32 user32 advapi32 oldnames
libraries.mingw += gdi32 comdlg32 imm32 winspool ws2_32 ole32 uuid user32 advapi32
darwin.ld.flags += -framework Carbon -framework SystemConfiguration
ifneq ($($(platform).$(arch).qt.version),)
libraries.linux += freetype Xext Xrender Xrandr Xfixes X11 fontconfig
libraries.dingux += png
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
	$(tools.moc) -nw $(addprefix -D,$(DEFINES)) $< -o $@

$(generated_dir)/%$(suffix.moc.cpp): %$(suffix.ui.h) | $(generated_dir)
	$(tools.moc) -nw $(addprefix -D,$(DEFINES)) $< -o $@

$(generated_dir)/%$(suffix.qrc.cpp): %$(suffix.qrc) | $(generated_dir)
	$(tools.rcc) -name $(basename $(notdir $<)) -o $@ $<

#disable dependencies generation for generated files
$(objects_dir)/$(call makeobj_name,%$(suffix.moc.cpp)): $(generated_dir)/%$(suffix.moc.cpp)
	$(call build_obj_cmd_nodeps,$(CURDIR)/$<,$@)

$(objects_dir)/$(call makeobj_name,%$(suffix.qrc.cpp)): $(generated_dir)/%$(suffix.qrc.cpp)
	$(call build_obj_cmd_nodeps,$(CURDIR)/$<,$@)
