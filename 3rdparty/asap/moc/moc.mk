MOC_INCLUDE = ../moc-2.4.4
MOC_PLUGIN_DIR = /usr/local/lib/moc/decoder_plugins

# no user-configurable paths below this line

ifndef DO
$(error Use "Makefile" instead of "moc.mk")
endif

asap-moc: libasap_decoder.so
.PHONY: asap-moc

libasap_decoder.so: $(call src,moc/libasap_decoder.c asap.[ch])
	$(CC) -I$(MOC_INCLUDE)
CLEAN += libasap_decoder.so

install-moc: libasap_decoder.so
	$(call INSTALL_PROGRAM,libasap_decoder.so,$(MOC_PLUGIN_DIR))
.PHONY: install-moc

uninstall-moc:
	$(RM) $(DESTDIR)$(MOC_PLUGIN_DIR)/libasap_decoder.so
.PHONY: uninstall-moc
