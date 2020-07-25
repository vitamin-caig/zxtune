GIT = git
TAR = tar
UNIX2DOS = $(DO)unix2dos <$< >$@
GREP = @grep -H

# no user-configurable paths below this line

VERSION = 3.2.0

ifndef DO
$(error Use "Makefile" instead of "release.mk")
endif

dist: \
	release/asap-$(VERSION)-air.air \
	release/asap-$(VERSION)-android.apk \
	release/asap-$(VERSION)-java.zip \
	release/asap-$(VERSION)-web.zip \
	release/asap-$(VERSION)-win32.msi \
	release/asap-$(VERSION)-win32.zip \
	release/asap-$(VERSION)-win64.msi \
	release/asap-$(VERSION)-wince-arm.cab \
	release/asap-$(VERSION)-wince-arm.zip \
	release/foo_asap-$(VERSION).fb2k-component \
	srcdist
.PHONY: dist

srcdist: $(srcdir)MANIFEST $(srcdir)README.html $(srcdir)asap.c $(srcdir)asap.h $(ASM6502_OBX)
	$(RM) release/asap-$(VERSION).tar.gz && $(TAR) -c --numeric-owner --owner=0 --group=0 --mode=644 -T MANIFEST --transform=s,,asap-$(VERSION)/, | $(SEVENZIP) -tgzip -si release/asap-$(VERSION).tar.gz
.PHONY: srcdist

$(srcdir)MANIFEST:
	$(DO)if test -e $(srcdir).git; then \
		($(GIT) ls-files | grep -vF .gitignore \
			&& echo MANIFEST && echo README.html && echo asap.c && echo asap.h \
			&& for obx in $(ASM6502_OBX); do echo $$obx; done) | sort -u >$@; \
	fi
.PHONY: $(srcdir)MANIFEST

release/asap-$(VERSION)-java.zip: release/COPYING.txt release/README_Java.html \
	java/asap2wav.jar java/j2me/asap_midlet.jad java/j2me/asap_midlet.jar java/asap.jar
	$(MAKEZIP)

release/asap-$(VERSION)-web.zip: release/COPYING.txt release/README_Web.html \
	flash/asap.swf java/asap_applet.jar javascript/asap.js $(srcdir)javascript/asapweb.js $(srcdir)javascript/binaryHttpRequest.js csharp/SilverASAP.xap
	$(MAKEZIP)

release/asap-$(VERSION)-win32.zip: release/COPYING.txt release/README_Windows.html \
	$(addprefix win32/,asapconv.exe asapscan.exe wasap.exe in_asap.dll foo_asap.dll gspasap.dll asap_dsf.dll install_dsf.bat uninstall_dsf.bat ASAP_Apollo.dll apokeysnd.dll xbmc_asap.dll xmp-asap.dll bass_asap.dll ASAPShellEx.dll asapplug.dll libasap_plugin.dll libgstasapdec.dll)
	$(MAKEZIP)

release/asap-$(VERSION)-wince-arm.zip: release/COPYING.txt release/README_WindowsCE.html \
	win32/wince/wasap.exe win32/wince/gspasap.dll
	$(MAKEZIP)

release/foo_asap-$(VERSION).fb2k-component: win32/foo_asap.dll
	$(MAKEZIP)

release/asap-vlc-$(VERSION)-osx.dmg: release/osx/libasap_plugin.dylib release/osx/plugins
	$(DO)hdiutil create -volname asap-vlc-$(VERSION)-osx -srcfolder release/osx -imagekey zlib-level=9 -ov $@

release/osx/libasap_plugin.dylib: libasap_plugin.dylib
	$(COPY) && chmod 644 $@
CLEANDIR += release/osx

release/osx/plugins:
	$(DO)ln -s /Applications/VLC.app/Contents/MacOS/plugins $@

deb:
	debuild -b -us -uc
.PHONY: deb

release/COPYING.txt: $(srcdir)COPYING
	$(UNIX2DOS)
CLEAN += release/COPYING.txt

release/README_Web.html: $(call src,README USAGE-WEB CREDITS)
	$(call ASCIIDOC,-a toc -a asapweb)
CLEAN += release/README_Web.html

release/README_Java.html: $(call src,README java/USAGE CREDITS)
	$(call ASCIIDOC,-a asapjava)
CLEAN += release/README_Java.html

release/README_JavaScript.html: $(call src,README javascript/USAGE CREDITS)
	$(call ASCIIDOC,-a asapjavascript)
CLEAN += release/README_JavaScript.html

release/README_Windows.html: $(call src,README win32/USAGE CREDITS)
	$(call ASCIIDOC,-a asapwin)
CLEAN += release/README_Windows.html

release/README_WindowsCE.html: $(call src,README win32/wince/USAGE CREDITS)
	$(call ASCIIDOC,-a asapwince)
CLEAN += release/README_WindowsCE.html

version:
	@echo ./release/release.mk: VERSION=$(VERSION)
	$(GREP) -m 1 ^ASAP $(srcdir)NEWS
	$(GREP) "<since>" $(srcdir)PORTS.xml | sort -ru | head -1
	$(GREP) Version: $(srcdir)asap.spec
	$(GREP) -m 1 ^asap $(srcdir)debian/changelog
	$(GREP) "int Version" $(srcdir)asapinfo.ci
	$(GREP) "VERSION =" $(srcdir)chksap.pl
	$(GREP) "Version=" $(srcdir)csharp/winrt/AppxManifest.xml
	$(GREP) android:versionName $(srcdir)java/android/AndroidManifest.xml
	$(GREP) about_title $(srcdir)java/android/res/values/strings.xml
	$(GREP) MIDlet-Version $(srcdir)java/j2me/MANIFEST.MF
	$(GREP) version $(srcdir)javascript/air/AIRASAP-app.xml
	$(GREP) ", v" $(srcdir)win32/rmt/apokeysnd_dll.c
.PHONY: version
