AUDACIOUS_VERSION := $(shell pkg-config --modversion audacious 2>/dev/null)
AUDACIOUS_CFLAGS = `pkg-config --cflags $(if $(filter 2.%,$(AUDACIOUS_VERSION)),gtk+-2.0 libmowgli,gtk+-3.0)`
AUDACIOUS_INPUT_PLUGIN_DIR = `pkg-config --variable=input_plugin_dir audacious`

# no user-configurable paths below this line

ifndef DO
$(error Use "Makefile" instead of "audacious.mk")
endif

asap-audacious: asapplug-$(AUDACIOUS_VERSION).so
.PHONY: asap-audacious

asapplug-$(AUDACIOUS_VERSION).so: $(call src,audacious/asapplug.c asap.[ch])
ifndef AUDACIOUS_VERSION
	$(error Please install audacious-dev or audacious-devel)
endif
	$(CC) $(AUDACIOUS_CFLAGS)
CLEAN += asapplug-*.so

install-audacious: asapplug-$(AUDACIOUS_VERSION).so
	$(call INSTALL_PROGRAM,asapplug-$(AUDACIOUS_VERSION).so,$(AUDACIOUS_INPUT_PLUGIN_DIR),asapplug.so)
.PHONY: install-audacious

uninstall-audacious:
	$(RM) $(DESTDIR)$(AUDACIOUS_INPUT_PLUGIN_DIR)/asapplug.so
.PHONY: uninstall-audacious
