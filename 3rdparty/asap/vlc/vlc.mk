# Linux

VLC_CFLAGS = -std=gnu99 -I/usr/include/vlc/plugins
VLC_DEMUX_PLUGIN_DIR = $(libdir)/vlc/plugins/demux

# OS X

VLC_OSX_CFLAGS = -std=gnu99 -I../plugins -dynamiclib -flat_namespace
VLC_OSX_PLUGIN_DIR = /Applications/VLC.app/Contents/MacOS/plugins

# no user-configurable paths below this line

ifndef DO
$(error Use "Makefile" instead of "vlc.mk")
endif

# Linux

asap-vlc: libasap_plugin.so
.PHONY: asap-vlc

libasap_plugin.so: $(call src,vlc/libasap_plugin.c asap.[ch])
	$(CC) $(VLC_CFLAGS)
CLEAN += libasap_plugin.so

install-vlc: libasap_plugin.so
	$(call INSTALL_PROGRAM,libasap_plugin.so,$(VLC_DEMUX_PLUGIN_DIR))
.PHONY: install-vlc

uninstall-vlc:
	$(RM) $(DESTDIR)$(VLC_DEMUX_PLUGIN_DIR)/libasap_plugin.so
.PHONY: uninstall-vlc

# OS X

asap-vlc-osx: libasap_plugin.dylib
.PHONY: asap-vlc-osx

libasap_plugin.dylib: $(call src,vlc/libasap_plugin.c asap.[ch])
	$(OSX_CC) $(VLC_OSX_CFLAGS)
CLEAN += libasap_plugin.dylib

install-vlc-osx: libasap_plugin.dylib
	$(call INSTALL_DATA,libasap_plugin.dylib,$(VLC_OSX_PLUGIN_DIR))
.PHONY: install-vlc-osx

uninstall-vlc-osx:
	$(RM) $(DESTDIR)$(VLC_OSX_PLUGIN_DIR)/libasap_plugin.dylib
.PHONY: uninstall-vlc-osx

