MADS = $(DO)mads -s -o:$@ $<

# no user-configurable paths below this line

ifndef DO
$(error Use "Makefile" instead of "6502.mk")
endif

ASM6502_PLAYERS = cmc cm3 cmr cms dlt mpt rmt4 rmt8 tmc tm2 fc
ASM6502_PLAYERS_OBX = $(ASM6502_PLAYERS:%=6502/%.obx)
ASM6502_OBX = $(ASM6502_PLAYERS_OBX) 6502/xexb.obx 6502/xexd.obx 6502/xexinfo.obx

6502/cmc.obx: $(srcdir)6502/cmc.asx
	$(XASM) -d CM3=0 -d CMR=0

6502/cm3.obx: $(srcdir)6502/cmc.asx
	$(XASM) -d CM3=1 -d CMR=0

6502/cmr.obx: $(srcdir)6502/cmc.asx
	$(XASM) -d CM3=0 -d CMR=1

6502/dlt.obx: $(srcdir)6502/dlt.as8
	$(MADS) -c

6502/rmt4.obx: $(srcdir)6502/rmt.asx
	$(XASM) -d STEREOMODE=0

6502/rmt8.obx: $(srcdir)6502/rmt.asx
	$(XASM) -d STEREOMODE=1

6502/fc.obx: $(srcdir)6502/fc.as8
	$(MADS)

6502/xexinfo.obx: $(srcdir)6502/xexinfo.asx
	$(XASM) -d TEST=0 -l

6502/xexinfo.xex: $(srcdir)6502/xexinfo.asx
	$(XASM) -d TEST=1

6502/%.obx: $(srcdir)6502/%.asx
	$(XASM) -l

CLEAN += $(ASM6502_OBX) 6502/fp3depk.obx 6502/xexinfo.xex 6502/*.lst
