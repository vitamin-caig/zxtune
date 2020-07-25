ANDROID_SDK = C:/bin/android-sdk-windows
ANDROID_JAR = $(ANDROID_SDK)/platforms/android-19/android.jar
ANDROID_BUILD_TOOLS = $(ANDROID_SDK)/build-tools/19.0.1
PROGUARD_JAR = C:/bin/proguard4.6/lib/proguard.jar

AAPT = $(ANDROID_BUILD_TOOLS)/aapt
DX = java -jar "$(ANDROID_BUILD_TOOLS)/lib/dx.jar" --no-strict
PROGUARD = $(DO)java -jar $(PROGUARD_JAR)
APKBUILDER = $(DO)java -classpath "$(ANDROID_SDK)/tools/lib/sdklib.jar" com.android.sdklib.build.ApkBuilderMain $@
JARSIGNER = $(DO)jarsigner -sigalg SHA1withDSA -digestalg SHA1
ZIPALIGN = $(DO)$(ANDROID_SDK)/tools/zipalign
ADB = $(ANDROID_SDK)/platform-tools/adb
ANDROID = $(ANDROID_SDK)/tools/android.bat
EMULATOR = $(ANDROID_SDK)/tools/emulator

# NDK is only needed for the command-line asapconv
# It ended up in this Makefile even though it's not Java
ANDROID_NDK = C:/bin/android-ndk-r8c
ANDROID_NDK_PLATFORM = $(ANDROID_NDK)/platforms/android-3/arch-arm
ANDROID_CC = $(DO)$(ANDROID_NDK)/toolchains/arm-linux-androideabi-4.6/prebuilt/windows/bin/arm-linux-androideabi-gcc --sysroot=$(ANDROID_NDK_PLATFORM) -s -O2 -Wall -o $@ $(INCLUDEOPTS) $(filter %.c,$^)

# no user-configurable paths below this line

ifndef DO
$(error Use "Makefile" instead of "android.mk")
endif

ANDROID_RELEASE = release/asap-$(VERSION)-android.apk

android-debug: java/android/AndroidASAP-debug.apk
.PHONY: android-debug

android-release: $(ANDROID_RELEASE)
.PHONY: android-release

android-install-emu: java/android/AndroidASAP-debug.apk
	$(ADB) -e install -r $<
.PHONY: android-install-emu

android-install-dev: $(ANDROID_RELEASE)
	$(ADB) -d install -r $<
.PHONY: android-install-dev

android-log-emu:
	$(ADB) -e logcat -d
.PHONY: android-log-emu

android-log-dev:
	$(ADB) -d logcat
.PHONY: android-log-dev

android-shell-emu:
	$(ADB) -e shell
.PHONY: android-shell-emu

android-shell-dev:
	$(ADB) -d shell
.PHONY: android-shell-dev

android-emu:
	$(EMULATOR) -avd myavd &
.PHONY: android-emu

android-push-release: $(ANDROID_RELEASE)
	$(ADB) -d push $(ANDROID_RELEASE) /sdcard/sap/
.PHONY: android-push-release

android-push-sap:
	$(ADB) -e push ../Drunk_Chessboard.sap /sdcard/
.PHONY: android-push-sap

android-create-avd:
	$(ANDROID) create avd -n myavd -t android-4 -c 16M
.PHONY: android-create-avd

$(ANDROID_RELEASE): java/android/AndroidASAP-unaligned.apk
	$(ZIPALIGN) -f 4 $< $@

java/android/AndroidASAP-unaligned.apk: java/android/AndroidASAP-unsigned.apk
	$(JARSIGNER) -storepass walsie -signedjar $@ $< pfusik-android
CLEAN += java/android/AndroidASAP-unaligned.apk

java/android/AndroidASAP-unsigned.apk: java/android/AndroidASAP-resources.apk java/android/classes.dex
	$(APKBUILDER) -u -z $< -f java/android/classes.dex
CLEAN += java/android/AndroidASAP-unsigned.apk

java/android/AndroidASAP-debug.apk: java/android/AndroidASAP-resources.apk java/android/classes.dex
	$(APKBUILDER) -z $< -f java/android/classes.dex
CLEAN += java/android/AndroidASAP-debug.apk

java/android/classes.dex: java/android/classes/net/sf/asap/Player.class
	$(DX) --dex --output=$@ java/android/classes
CLEAN += java/android/classes.dex

#java/android/classes.dex: java/android/classes.jar
#	$(DX) --dex --output=$@ $<

java/android/classes.jar: $(srcdir)java/android/proguard.cfg java/android/classes/net/sf/asap/Player.class
	$(PROGUARD) -injars java/android/classes -outjars $@ -libraryjars $(ANDROID_JAR) @$<

java/android/classes/net/sf/asap/Player.class: $(addprefix $(srcdir)java/android/,AATRStream.java FileContainer.java FileSelector.java MediaButtonEventReceiver.java Player.java PlayerService.java Util.java ZipInputStream.java) java/android/AndroidASAP-resources.apk java/src/net/sf/asap/ASAP.java
	$(JAVAC) -d java/android/classes -bootclasspath $(ANDROID_JAR) $(addprefix $(srcdir)java/android/,AATRStream.java FileContainer.java FileSelector.java MediaButtonEventReceiver.java Player.java PlayerService.java Util.java ZipInputStream.java) java/android/gen/net/sf/asap/R.java java/src/net/sf/asap/*.java
CLEANDIR += java/android/classes

# Also generates java/android/gen/net/sf/asap/R.java
java/android/AndroidASAP-resources.apk: $(addprefix $(srcdir)java/android/,AndroidManifest.xml res/drawable/icon.png res/layout/fileinfo_list_item.xml res/layout/filename_list_item.xml res/layout/playing.xml res/menu/file_selector.xml res/menu/playing.xml res/values/strings.xml res/values/themes.xml) $(JAVA_OBX)
	$(DO)mkdir -p java/android/gen && $(AAPT) p -f -m -M $< -I $(ANDROID_JAR) -S $(srcdir)java/android/res -F $@ -J java/android/gen java/obx
CLEAN += java/android/AndroidASAP-resources.apk java/android/gen/net/sf/asap/R.java

android-push-asapconv: java/android/asapconv
	$(ADB) -d push java/android/asapconv /data/local/tmp/
.PHONY: android-push-asapconv

java/android/asapconv: $(call src,asapconv.c asap.[ch])
	$(ANDROID_CC)
CLEAN += java/android/asapconv
