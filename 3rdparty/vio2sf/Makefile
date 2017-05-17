
CFLAGS = -c
CXXFLAGS = -c

OBJS = desmume/FIFO.o desmume/GPU.o desmume/MMU.o desmume/NDSSystem.o desmume/SPU.o desmume/arm_instructions.o desmume/armcpu.o desmume/barray.o desmume/bios.o desmume/cp15.o desmume/isqrt.o desmume/matrix.o desmume/mc.o desmume/resampler.o desmume/state.o desmume/thumb_instructions.o

OPTS = -O3 -I.

all: libvio2sf.a

libvio2sf.a : $(OBJS)
	$(AR) rcs $@ $^

trim2sflib: trim2sflib.o libvio2sf.a ../../../psflib/libpsflib.a
	$(CXX) -o $@ $^ -lz

.c.o:
	$(CC) $(CFLAGS) $(OPTS) -o $@ $*.c

.cpp.o:
	$(CC) $(CXXFLAGS) $(OPTS) -o $@ $*.cpp

clean:
	rm -f $(OBJS) libvio2sf.a trim2sflib.o trim2sflib > /dev/null

