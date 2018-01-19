GSTREAMER_CFLAGS = `pkg-config --cflags --libs gstreamer-0.10`
GSTREAMER_PLUGIN_DIR = `pkg-config --variable=pluginsdir gstreamer-0.10`

# no user-configurable paths below this line

ifndef DO
$(error Use "Makefile" instead of "gstreamer.mk")
endif

asap-gstreamer: libgstasapdec.so
.PHONY: asap-gstreamer

libgstasapdec.so: $(call src,gstreamer/gstasapdec.c asap.[ch])
	$(CC) $(GSTREAMER_CFLAGS)
CLEAN += libgstasapdec.so

install-gstreamer: libgstasapdec.so
	$(call INSTALL_PROGRAM,libgstasapdec.so,$(GSTREAMER_PLUGIN_DIR))
.PHONY: install-gstreamer

uninstall-gstreamer:
	$(RM) $(DESTDIR)$(GSTREAMER_PLUGIN_DIR)/libgstasapdec.so
.PHONY: uninstall-gstreamer
