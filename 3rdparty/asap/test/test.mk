ACIDSAP = ../Acid800/out/Release/AcidSAP/standalone

# no user-configurable paths below this line

TESTS = antic_nmires antic_nmist antic_vcount_ntsc antic_vcount_pal antic_wsync cpu_anx cpu_decimal cpu_las cpu_shx gtia_consol pokey_irqst pokey_pot pokey_potst pokey_random pokey_timerirq
TESTS_SAP = $(TESTS:%=test/%.sap)

check: asapscan $(ACIDSAP) $(TESTS_SAP)
	@export passed=0 total=0; \
		for name in $(ACIDSAP)/*.sap; do \
			echo -n \*\ ; ./asapscan -a "$$name"; \
			passed=$$(($$passed+!$$?)); total=$$(($$total+1)); \
		done; \
		echo PASSED $$passed of $$total tests
	@export passed=0 total=0; \
		for name in $(TESTS_SAP); do \
			echo -n \*\ ; ./asapscan -a "$$name"; \
			passed=$$(($$passed+!$$?)); total=$$(($$total+1)); \
		done; \
		echo PASSED $$passed of $$total tests
.PHONY: check

test/%.sap: $(srcdir)test/%.asx
	$(XASM) -d SAP=1

test/%_ntsc.sap: $(srcdir)test/%.asx
	$(XASM) -d SAP=1 -d NTSC=1

test/%_pal.sap: $(srcdir)test/%.asx
	$(XASM) -d SAP=1 -d NTSC=0

test/%.xex: $(srcdir)test/%.asx
	$(XASM) -d SAP=0

CLEAN += test/*.sap test/*.xex

test/timevsnative: $(call src,test/timevsnative.c asap.[ch])
	$(CC)
CLEAN += test/timevsnative

test/loadsap.exe: $(call src,test/loadsap.cs csharp/asap.cs)
	$(CSC)
CLEAN += test/loadsap.exe

test/ultrasap.exe: $(call src,test/ultrasap.cs csharp/asap.cs)
	$(CSC)
CLEAN += test/ultrasap.exe

test/crashsap.exe: $(call src,test/crashsap.cs csharp/asap.cs)
	$(CSC)
CLEAN += test/crashsap.exe

include $(srcdir)test/benchmark/benchmark.mk
include $(srcdir)test/disasm/disasm.mk
