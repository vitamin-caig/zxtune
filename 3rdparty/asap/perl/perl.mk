# no user-configurable paths below this line

ifndef DO
$(error Use "Makefile" instead of "perl.mk")
endif

perl: perl/Asap.pm
.PHONY: perl

perl/Asap.pm: $(call src,asap.ci asap6502.ci asapinfo.ci cpu6502.ci pokey.ci) $(ASM6502_PLAYERS_OBX)
	$(CITO)
CLEAN += perl/Asap.pm
