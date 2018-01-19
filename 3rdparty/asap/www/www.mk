ifndef DO
$(error Use "Makefile" instead of "www.mk")
endif

www: www/index.html www/NEWS.html www/applet.html www/flash.html www/javascript.html www/silverlight.html www/apokeysnd.dll \
	www/asap.swf www/asap_applet.jar www/asap.js www/asapweb.js www/binaryHttpRequest.js www/SilverASAP.xap \
	www/favicon.ico www/PORTS.xml www/PORTS.xsl
.PHONY: www

www/index.html: $(call src,README CREDITS)
	$(call ASCIIDOC,-a asapwww -a asapports)

www/NEWS.html: $(srcdir)NEWS
	$(call ASCIIDOC,)

www/applet.html: $(srcdir)www/applet.txt
	$(call ASCIIDOC,)

www/flash.html: $(srcdir)www/flash.txt
	$(call ASCIIDOC,)

www/javascript.html: $(srcdir)www/javascript.txt
	$(DO)asciidoc -o - $< | sed -e "s/527bbd;/c02020;/" | xmllint --dropdtd --nonet -o $@ -

www/silverlight.html: $(srcdir)www/silverlight.txt
	$(call ASCIIDOC,)

www/apokeysnd.dll: win32/apokeysnd.dll
	$(COPY)

www/asap.swf: flash/asap.swf
	$(COPY)

www/asap_applet.jar: java/asap_applet.jar
	$(COPY)

www/asap.js: javascript/asap.js
	$(COPY)

www/asapweb.js: $(srcdir)javascript/asapweb.js
	$(COPY)

www/binaryHttpRequest.js: $(srcdir)javascript/binaryHttpRequest.js
	$(COPY)

www/SilverASAP.xap: csharp/SilverASAP.xap
	$(COPY)

www/favicon.ico: $(srcdir)win32/wasap/wasap.ico
	$(COPY)

www/PORTS.xml: $(srcdir)PORTS.xml
	$(COPY)

www/PORTS.xsl: $(srcdir)PORTS.xsl
	$(COPY)
