JSC = $(DO)jsc -nologo -w:2 -out:$@ $<

# no user-configurable paths below this line

ifndef DO
$(error Use "Makefile" instead of "javascript.mk")
endif

javascript: javascript/asap2wav.js
.PHONY: javascript

javascript/asap2wav.exe: javascript/asap2wav.js
	$(JSC)
CLEAN += javascript/asap2wav.exe

javascript/asap2wav.js: javascript/asap.js $(srcdir)javascript/asap2wav1.js
	$(DO)cat $^ > $@
CLEAN += javascript/asap2wav.js

javascript/asap.js: $(call src,asap.ci asap6502.ci asapinfo.ci cpu6502.ci pokey.ci) $(ASM6502_PLAYERS_OBX)
	$(CITO)
CLEAN += javascript/asap.js

include $(srcdir)javascript/air/air.mk
