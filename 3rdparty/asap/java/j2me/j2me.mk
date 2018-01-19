WTK = C:/bin/Java_ME_platform_SDK_3.2
ME_CLASSES = $(WTK)/lib/cldc_1.1.jar;$(WTK)/lib/midp_2.0.jar;$(WTK)/lib/jsr75_1.0.jar
PREVERIFY = $(DO)"$(WTK)/bin/preverify"
ME_EMU = "$(WTK)/bin/emulator"

# no user-configurable paths below this line

ifndef DO
$(error Use "Makefile" instead of "j2me.mk")
endif

midlet-emu: java/j2me/asap_midlet.jad
	$(ME_EMU) -Xdescriptor:$<

java/j2me/asap_midlet.jad: $(srcdir)java/j2me/MANIFEST.MF java/j2me/asap_midlet.jar
	$(DO)(echo MIDlet-Jar-URL: asap_midlet.jar && echo MIDlet-Jar-Size: `stat -c %s java/j2me/asap_midlet.jar` && cat $<) > $@
CLEAN += java/j2me/asap_midlet.jad

java/j2me/asap_midlet.jar: $(srcdir)java/j2me/MANIFEST.MF java/j2me/preverified/ASAPMIDlet.class $(JAVA_OBX)
	$(JAR) cfm $@ $< -C java/j2me/preverified . $(ASM6502_PLAYERS:%=-C java/obx net/sf/asap/%.obx)
CLEAN += java/j2me/asap_midlet.jar

java/j2me/preverified/ASAPMIDlet.class: java/classes/ASAPMIDlet.class
	$(PREVERIFY) -classpath "$(ME_CLASSES);java/classes" -d $(@D) ASAPMIDlet FileList ASAPInputStream net.sf.asap.ASAP net.sf.asap.ASAP6502 net.sf.asap.ASAPInfo net.sf.asap.ASAPModuleType net.sf.asap.ASAPSampleFormat net.sf.asap.Cpu6502 net.sf.asap.NmiStatus net.sf.asap.Pokey net.sf.asap.PokeyPair
CLEANDIR += java/j2me/preverified

java/classes/ASAPMIDlet.class: $(srcdir)java/j2me/ASAPMIDlet.java java/classes/net/sf/asap/ASAP.class
	$(JAVAC) -d $(@D) -source 1.2 -bootclasspath "$(ME_CLASSES)" -classpath java/classes $<
