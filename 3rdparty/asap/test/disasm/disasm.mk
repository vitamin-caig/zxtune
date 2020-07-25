disasm: $(addprefix test/disasm/,asap.s asap64.s asap.html asap.jasm asap.abc)
.PHONY: disasm

test/disasm/asap.s: $(call src,asap.[ch])
	$(WIN32_CC) -S
CLEAN += test/disasm/asap.s

test/disasm/asap64.s: $(call src,asap.[ch])
	$(WIN64_CC) -S
CLEAN += test/disasm/asap64.s

test/disasm/asap.html: csharp/asap2wav.exe
	-$(DO)"c:/Program Files (x86)/Microsoft SDKs/Windows/v7.0A/Bin/ildasm.exe" /output=$@ /html /nobar /source $<
CLEAN += test/disasm/asap.html

test/disasm/asap.jasm: java/classes/net/sf/asap
	$(DO)javap -c -private -classpath java/classes net.sf.asap.ASAP net.sf.asap.ASAPInfo net.sf.asap.Cpu6502 net.sf.asap.Pokey net.sf.asap.PokeyPair > $@
CLEAN += test/disasm/asap.jasm

test/disasm/asap.abc: flash/asap.swf
	$(DO)swfdump -abc $< > $@
CLEAN += test/disasm/asap.abc

test/disasm/asap.dexdump: java/android/classes/net/sf/asap/Player.class
	$(DO)java -jar "c:/bin/android-sdk-windows/platforms/android-8//tools/lib/dx.jar" --dex --dump-to=$@ java/android/classes/
CLEAN += test/disasm/asap.dexdump
