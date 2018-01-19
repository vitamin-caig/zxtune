JRE = C:/Program Files/Java/jre7
JAVAC = $(DO)javac
JAR = $(DO)jar
JAVADOC = $(DO)javadoc

# no user-configurable paths below this line

ifndef DO
$(error Use "Makefile" instead of "java.mk")
endif

JAVA_OBX = $(ASM6502_PLAYERS:%=java/obx/net/sf/asap/%.obx)
JAR_COMMON = -C java/classes net $(ASM6502_PLAYERS:%=-C java/obx net/sf/asap/%.obx)

java/asap2wav.jar: $(srcdir)java/asap2wav.MF java/classes/ASAP2WAV.class $(JAVA_OBX)
	$(JAR) cfm $@ $< -C java/classes ASAP2WAV.class $(JAR_COMMON)
CLEAN += java/asap2wav.jar

java/classes/ASAP2WAV.class: $(srcdir)java/ASAP2WAV.java java/classes/net/sf/asap/ASAP.class
	$(JAVAC) -d $(@D) -source 1.2 -classpath java/classes $<

java/asap_applet.jar: java/classes/ASAPApplet.class $(JAVA_OBX)
	$(JAR) cf $@ -C java/classes ASAPApplet.class $(JAR_COMMON)
CLEAN += java/asap_applet.jar

java/classes/ASAPApplet.class: $(srcdir)java/ASAPApplet.java java/classes/net/sf/asap/ASAP.class
	$(JAVAC) -d $(@D) -source 1.2 -classpath "$(JRE)/lib/plugin.jar;java/classes" $<

java/asap.jar: java/classes/net/sf/asap/ASAP.class $(JAVA_OBX)
	$(JAR) cf $@ $(JAR_COMMON)
CLEAN += java/asap.jar

java/obx/net/sf/asap/%.obx: 6502/%.obx
	$(COPY)
CLEAN += $(JAVA_OBX)

java/classes/net/sf/asap/ASAP.class: $(srcdir)java/ASAPMusicRoutine.java java/src/net/sf/asap/ASAP.java
	$(JAVAC) -d java/classes -source 1.2 $(srcdir)java/ASAPMusicRoutine.java java/src/net/sf/asap/*.java
CLEANDIR += java/classes

java/src/net/sf/asap/ASAP.java: $(call src,asap.ci asap6502.ci asapinfo.ci asapwriter.ci cpu6502.ci pokey.ci aatr.ci) $(ASM6502_OBX)
	$(CITO) -n net.sf.asap
CLEANDIR += java/src

java/doc: java/src/net/sf/asap/ASAP.java $(srcdir)java/ASAPMusicRoutine.java
	$(JAVADOC) -d $@ java/src/net/sf/asap/*.java $(srcdir)java/ASAPMusicRoutine.java
CLEANDIR += java/doc

include $(srcdir)java/android/android.mk
include $(srcdir)java/j2me/j2me.mk
