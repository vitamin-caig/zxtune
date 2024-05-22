
AUTOMAKE_OPTIONS = foreign subdir-objects

if ENABLE_TEST

AM_CPPFLAGS = \
-I$(top_srcdir)/src \
-I$(top_builddir)/src/builders/residfp-builder/residfp \
$(UNITTEST_CFLAGS)

AM_LDFLAGS = $(UNITTEST_LIBS)

TESTS = \
TestEnvelopeGenerator \
TestWaveformGenerator \
TestSpline \
TestDac \
TestPSID \
TestMUS \
TestMos6510 \
TestResampler

check_PROGRAMS = $(TESTS)

TestEnvelopeGenerator_SOURCES = \
Main.cpp \
TestEnvelopeGenerator.cpp

TestWaveformGenerator_SOURCES = \
Main.cpp \
TestWaveformGenerator.cpp

TestSpline_SOURCES = \
Main.cpp \
TestSpline.cpp

TestDac_SOURCES = \
Main.cpp \
TestDac.cpp

TestPSID_SOURCES = \
Main.cpp \
TestPSID.cpp
TestPSID_LDADD = $(top_builddir)/src/libsidplayfp.la

TestMUS_SOURCES = \
Main.cpp \
TestMUS.cpp
TestMUS_LDADD = $(top_builddir)/src/libsidplayfp.la

TestMos6510_SOURCES = \
Main.cpp \
TestMos6510.cpp

TestResampler_SOURCES = \
Main.cpp \
TestResampler.cpp

endif
