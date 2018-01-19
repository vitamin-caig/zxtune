MXMLC = $(DO)mxmlc -o $@ -compiler.optimize -compiler.warn-duplicate-variable-def=false -static-link-runtime-shared-libraries -target-player 10 $<
# -compiler.debug -compiler.warn-no-type-decl=false
ASDOC = $(DO)asdoc

# no user-configurable paths below this line

ifndef DO
$(error Use "Makefile" instead of "flash.mk")
endif

flash/asap.swf: $(srcdir)flash/ASAPPlayer.as flash/net/sf/asap/ASAP.as $(ASM6502_PLAYERS_OBX)
	$(MXMLC) -source-path flash 6502
CLEAN += flash/asap.swf

flash/net/sf/asap/ASAP.as: $(call src,asap.ci asap6502.ci asapinfo.ci cpu6502.ci pokey.ci) $(ASM6502_PLAYERS_OBX)
	$(CITO) -n net.sf.asap -D FLASH
CLEAN += flash/net/sf/asap/*.as

debug-flash:
	tail -f "${APPDATA}\Macromedia\Flash Player\Logs\flashlog.txt"
.PHONY: debug-flash

flash/doc: flash/net/sf/asap/ASAP.as
	$(ASDOC) -compiler.warn-duplicate-variable-def=false -compiler.warn-const-not-initialized=false -output=$@ -window-title="ASAP ActionScript API" -main-title="ASAP (Another Slight Atari Player) ActionScript API" -doc-sources=$(<D)
CLEANDIR += flash/doc
