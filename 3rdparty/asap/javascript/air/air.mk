ADL = adl.exe $<
ADT = $(DO)adt.bat -package -storetype pkcs12 -keystore $(srcdir)javascript/air/pfusik-air.pfx -storepass walsie -tsa none $@

# no user-configurable paths below this line

ifndef DO
$(error Use "Makefile" instead of "air.mk")
endif

run-air: $(srcdir)javascript/air/AIRASAP-app.xml javascript/air/asap.js
	$(ADL)
.PHONY: run-air

release/asap-$(VERSION)-air.air: $(addprefix $(srcdir)javascript/air/,AIRASAP-app.xml airasap.html airasap.js asap16x16.png asap32x32.png) javascript/air/asap.js
	$(ADT) $< -C $(srcdir)javascript/air airasap.html airasap.js asap16x16.png asap32x32.png -C javascript/air asap.js

javascript/air/asap.js: $(call src,asap.ci asap6502.ci asapinfo.ci cpu6502.ci pokey.ci) $(ASM6502_PLAYERS_OBX)
	$(CITO) -D FLASH
CLEAN += javascript/air/asap.js
