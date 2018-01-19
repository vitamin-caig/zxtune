XMMS_CFLAGS = `xmms-config --cflags`
XMMS_LIBS = `xmms-config --libs`
XMMS_INPUT_PLUGIN_DIR = `xmms-config --input-plugin-dir`
XMMS_USER_PLUGIN_DIR = $(HOME)/.xmms/Plugins

# no user-configurable paths below this line

ifndef DO
$(error Use "Makefile" instead of "xmms.mk")
endif

asap-xmms: libasap-xmms.so
.PHONY: asap-xmms

libasap-xmms.so: $(call src,xmms/libasap-xmms.c asap.[ch])
	$(CC) $(XMMS_CFLAGS) -Wl,--version-script=$(srcdir)xmms/libasap-xmms.map $(XMMS_LIBS)
CLEAN += libasap-xmms.so

install-xmms: libasap-xmms.so
	$(call INSTALL_PROGRAM,libasap-xmms.so,$(XMMS_INPUT_PLUGIN_DIR))
.PHONY: install-xmms

uninstall-xmms:
	$(RM) $(DESTDIR)$(XMMS_INPUT_PLUGIN_DIR)/libasap-xmms.so
.PHONY: uninstall-xmms

install-xmms-user: libasap-xmms.so
	$(call INSTALL_PROGRAM,libasap-xmms.so,$(XMMS_USER_PLUGIN_DIR))
.PHONY: install-xmms-user

uninstall-xmms-user:
	$(RM) $(DESTDIR)$(XMMS_USER_PLUGIN_DIR)/libasap-xmms.so
.PHONY: uninstall-xmms-user
