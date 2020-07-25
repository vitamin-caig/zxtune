GME_PATH = ../game-music-emu
SAP_PATH = ../sap.src

benchmark: test/benchmark/BENCHMARK.html
.PHONY: benchmark

test/benchmark/BENCHMARK.html: test/benchmark/BENCHMARK.txt
	$(call ASCIIDOC,)

test/benchmark/BENCHMARK.txt: $(srcdir)test/benchmark/benchmark.pl win32/msvc/asapconv.exe win32/asapconv.exe win32/x64/asapconv.exe java/asap2wav.jar csharp/asap2wav.exe d/asap2wav.exe test/benchmark/gme_benchmark.exe test/benchmark/sap_benchmark.exe
	perl $< > $@

test/benchmark/gme_benchmark.exe: $(call src,test/benchmark/gme_benchmark.c asap.[ch]) $(GME_PATH)/gme/*.cpp $(GME_PATH)/gme/*.h
	$(WIN32_CXX)
CLEAN += test/benchmark/gme_benchmark.exe

test/benchmark/sap_benchmark.exe: $(call src,test/benchmark/sap_benchmark.cpp asap.[ch]) $(SAP_PATH)/pokey0.cpp $(SAP_PATH)/pokey1.cpp $(SAP_PATH)/sapCpu.cpp $(SAP_PATH)/sapEngine.cpp $(SAP_PATH)/sapLib.h $(SAP_PATH)/sapPokey.cpp
	$(WIN32_CXX)
CLEAN += test/benchmark/sap_benchmark.exe

profile: gmon.out
	gprof -bpQ test/benchmark/asapconv-profile.exe

gmon.out: test/benchmark/asapconv-profile.exe
	$(DO)./test/benchmark/asapconv-profile.exe -b -o .wav test/benchmark/Drunk_Chessboard.sap
CLEAN += gmon.out

test/benchmark/asapconv-profile.exe: $(call src,asapconv.c asap.[ch])
	$(DO)mingw32-gcc -O2 -Wall -o $@ -pg $(filter-out %.h,$^)
	# avoid -s
CLEAN += test/benchmark/asapconv-profile.exe
