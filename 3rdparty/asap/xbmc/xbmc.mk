XBMC_PLUGIN_DIR = /usr/lib/xbmc/system/players/paplayer

# no user-configurable paths below this line

ifndef DO
$(error Use "Makefile" instead of "xbmc.mk")
endif

XBMC_WRAP_FUNCTIONS = malloc realloc free fopen fread fclose

asap-xbmc: xbmc_asap-i486-linux.so
.PHONY: asap-xbmc

xbmc_asap-i486-linux.so: $(call src,xbmc/xbmc_asap.c asap.[ch] xbmc/wrapper.c)
	$(CC) $(XBMC_WRAP_FUNCTIONS:%=-Wl,-wrap,%)
CLEAN += xbmc_asap-i486-linux.so

install-xbmc: xbmc_asap-i486-linux.so
	$(call INSTALL_DATA,xbmc_asap-i486-linux.so,$(XBMC_PLUGIN_DIR))
.PHONY: install-xbmc

uninstall-xbmc:
	$(RM) $(DESTDIR)$(XBMC_PLUGIN_DIR)/xbmc_asap-i486-linux.so
.PHONY: uninstall-xbmc

# I wasn't able to run the OS X plugin :(

asap-xbmc-osx: xbmc_asap-x86-osx.so
.PHONY: asap-xbmc-osx

xbmc_asap-x86-osx.so: $(call src,xbmc/xbmc_asap.c asap.[ch] xbmc/wrapper.c)
	$(OSX_CC) -bundle $(foreach func,$(XBMC_WRAP_FUNCTIONS),-Wl,-alias,___wrap_$(func),_$(func))
CLEAN += xbmc_asap-x86-osx.so
