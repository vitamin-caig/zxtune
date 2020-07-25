DC = $(DO)dmd -release -inline -O $(if $(WINDIR),-of$(subst /,\\,$@) $(subst /,\\,$^),-of$@ $^)

# no user-configurable paths below this line

ifndef DO
$(error Use "Makefile" instead of "d.mk")
endif

d: d/asap2wav.exe d/asapplay.exe
.PHONY: d

d/asap2wav.exe: $(srcdir)d/asap2wav.d d/asap.d
	$(DC)
CLEAN += d/asap2wav.exe d/asap2wav.obj asap2wav.map

d/asapplay.exe: $(call src,d/asapplay.d d/waveout.d d/alsa/pcm.d) d/asap.d
	$(DC)
CLEAN += d/asapplay.exe d/asapplay.obj asapplay.map

d/asap.d: $(call src,asap.ci asap6502.ci asapinfo.ci cpu6502.ci pokey.ci) $(ASM6502_PLAYERS_OBX)
	$(CITO)
CLEAN += d/asap.d
