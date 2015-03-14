
ACLOCAL_AMFLAGS = -I m4

AUTOMAKE_OPTIONS = foreign subdir-objects


if MINGW32
  W32_LDFLAGS = -no-undefined
endif

if HAVE_VISIBILITY
  AM_CXXFLAGS = -fvisibility=hidden -fvisibility-inlines-hidden
endif

AM_CPPFLAGS = -I $(top_builddir)/builders/residfp-builder \
-I $(top_builddir)/builders/residfp-builder/residfp \
-I $(top_builddir)/builders/resid-builder \
-I $(top_builddir)/builders/resid-builder/resid \
-I $(top_builddir)/sidplayfp \
-I $(top_srcdir)/sidplayfp \
@debug_flags@

#=========================================================
EXTRA_DIST = \
docs/mainpage.dox \
sidplayfp/psiddrv.a65 \
builders/residfp-builder/residfp/AUTHORS \
builders/residfp-builder/residfp/COPYING \
builders/residfp-builder/residfp/README \
builders/resid-builder/resid/AUTHORS \
builders/resid-builder/resid/ChangeLog \
builders/resid-builder/resid/COPYING \
builders/resid-builder/resid/configure.in \
builders/resid-builder/resid/INSTALL \
builders/resid-builder/resid/Makefile.am \
builders/resid-builder/resid/NEWS \
builders/resid-builder/resid/README \
builders/resid-builder/resid/README.VICE \
builders/resid-builder/resid/THANKS \
builders/resid-builder/resid/TODO \
$(noinst_DATA) \
$(noinst_SCRIPTS)

#=========================================================
# Libraries
lib_LTLIBRARIES = \
sidplayfp/libsidplayfp.la \
sidplayfp/libstilview.la

noinst_LTLIBRARIES = \
builders/residfp-builder/residfp/libresidfp.la \
builders/resid-builder/resid/libresid.la \
builders/residfp-builder/libsidplayfp-residfp.la \
builders/resid-builder/libsidplayfp-resid.la

if HARDSID
  noinst_LTLIBRARIES += builders/hardsid-builder/libsidplayfp-hardsid.la
endif

#=========================================================
# libsidplayfp
sidplayfp_libsidplayfp_la_SOURCES = \
sidplayfp/config.cpp \
sidplayfp/EventScheduler.cpp \
sidplayfp/EventScheduler.h \
sidplayfp/player.cpp \
sidplayfp/player.h \
sidplayfp/psiddrv.cpp \
sidplayfp/psiddrv.h \
sidplayfp/psiddrv.bin \
sidplayfp/mixer.cpp \
sidplayfp/mixer.h \
sidplayfp/poweron.bin \
sidplayfp/reloc65.cpp \
sidplayfp/reloc65.h \
sidplayfp/sidbuilder.cpp \
sidplayfp/SidConfig.cpp \
sidplayfp/sidmd5.h \
sidplayfp/sidmemory.h \
sidplayfp/sidplayfp.cpp \
sidplayfp/SidInfoImpl.h \
sidplayfp/SidTune.cpp \
sidplayfp/romCheck.h \
sidplayfp/sidemu.cpp \
sidplayfp/sidemu.h \
sidplayfp/sidendian.h \
sidplayfp/sidrandom.h \
sidplayfp/stringutils.h \
sidplayfp/c64/Banks/Bank.h \
sidplayfp/c64/c64cpu.h \
sidplayfp/c64/c64cia.h \
sidplayfp/c64/c64env.h \
sidplayfp/c64/c64sid.h \
sidplayfp/c64/c64vic.h \
sidplayfp/c64/c64.cpp \
sidplayfp/c64/c64.h \
sidplayfp/c64/component.h \
sidplayfp/c64/mmu.cpp \
sidplayfp/c64/mmu.h \
sidplayfp/c64/Banks/ColorRAMBank.h \
sidplayfp/c64/Banks/DisconnectedBusBank.h \
sidplayfp/c64/Banks/ExtraSidBank.h \
sidplayfp/c64/Banks/IOBank.h \
sidplayfp/c64/Banks/NullSid.h \
sidplayfp/c64/Banks/SidBank.h \
sidplayfp/c64/Banks/SystemRAMBank.h \
sidplayfp/c64/Banks/SystemROMBanks.h \
sidplayfp/c64/Banks/ZeroRAMBank.h \
sidplayfp/c64/VIC_II/mos656x.cpp \
sidplayfp/c64/VIC_II/mos656x.h \
sidplayfp/c64/VIC_II/lightpen.h \
sidplayfp/c64/VIC_II/sprites.h \
sidplayfp/c64/CPU/flags.h \
sidplayfp/c64/CPU/mos6510.cpp \
sidplayfp/c64/CPU/mos6510.h \
sidplayfp/c64/CPU/mos6510debug.cpp \
sidplayfp/c64/CPU/mos6510debug.h \
sidplayfp/c64/CPU/opcodes.h \
sidplayfp/c64/CIA/mos6526.cpp \
sidplayfp/c64/CIA/mos6526.h \
sidplayfp/c64/CIA/timer.cpp \
sidplayfp/c64/CIA/timer.h \
sidplayfp/c64/CIA/tod.cpp \
sidplayfp/c64/CIA/tod.h \
sidplayfp/sidtune/MUS.cpp \
sidplayfp/sidtune/MUS.h \
sidplayfp/sidtune/p00.cpp \
sidplayfp/sidtune/p00.h \
sidplayfp/sidtune/prg.cpp \
sidplayfp/sidtune/prg.h \
sidplayfp/sidtune/PSID.cpp \
sidplayfp/sidtune/PSID.h \
sidplayfp/sidtune/sidplayer.bin \
sidplayfp/sidtune/SidTuneBase.cpp \
sidplayfp/sidtune/SidTuneBase.h \
sidplayfp/sidtune/SidTuneCfg.h \
sidplayfp/sidtune/SidTuneInfoImpl.h \
sidplayfp/sidtune/SidTuneTools.cpp \
sidplayfp/sidtune/SidTuneTools.h \
sidplayfp/sidtune/SmartPtr.h \
utils/iniParser.cpp \
utils/iniParser.h \
utils/SidDatabase.cpp \
utils/MD5/MD5.cpp \
utils/MD5/MD5.h \
utils/MD5/MD5_Defs.h

sidplayfp_libsidplayfp_la_LDFLAGS = -version-info $(LIBSIDPLAYVERSION) $(W32_LDFLAGS)

sidplayfp_libsidplayfp_ladir = $(includedir)/sidplayfp

sidplayfp_libsidplayfp_la_HEADERS = \
sidplayfp/siddefs.h \
sidplayfp/event.h \
sidplayfp/SidConfig.h \
sidplayfp/SidInfo.h \
sidplayfp/SidTuneInfo.h \
sidplayfp/sidbuilder.h \
sidplayfp/sidplayfp.h \
sidplayfp/SidTune.h \
utils/SidDatabase.h

nodist_sidplayfp_libsidplayfp_la_HEADERS = \
sidplayfp/sidversion.h

sidplayfp_libsidplayfp_la_LIBADD = \
builders/residfp-builder/libsidplayfp-residfp.la \
builders/resid-builder/libsidplayfp-resid.la

if HARDSID
  sidplayfp_libsidplayfp_la_LIBADD += builders/hardsid-builder/libsidplayfp-hardsid.la
endif

#=========================================================
# residfp

builders_residfp_builder_residfp_libresidfp_la_SOURCES = \
builders/residfp-builder/residfp/array.h \
builders/residfp-builder/residfp/Dac.cpp \
builders/residfp-builder/residfp/Dac.h \
builders/residfp-builder/residfp/Integrator.cpp \
builders/residfp-builder/residfp/Integrator.h \
builders/residfp-builder/residfp/EnvelopeGenerator.cpp \
builders/residfp-builder/residfp/EnvelopeGenerator.h \
builders/residfp-builder/residfp/ExternalFilter.cpp \
builders/residfp-builder/residfp/ExternalFilter.h \
builders/residfp-builder/residfp/FilterModelConfig.cpp \
builders/residfp-builder/residfp/FilterModelConfig.h \
builders/residfp-builder/residfp/Filter.cpp \
builders/residfp-builder/residfp/Filter.h \
builders/residfp-builder/residfp/Filter8580.cpp \
builders/residfp-builder/residfp/Filter8580.h \
builders/residfp-builder/residfp/Filter6581.cpp \
builders/residfp-builder/residfp/Filter6581.h \
builders/residfp-builder/residfp/OpAmp.cpp \
builders/residfp-builder/residfp/OpAmp.h \
builders/residfp-builder/residfp/Potentiometer.h \
builders/residfp-builder/residfp/SID.cpp \
builders/residfp-builder/residfp/SID.h \
builders/residfp-builder/residfp/Spline.cpp \
builders/residfp-builder/residfp/Spline.h \
builders/residfp-builder/residfp/Voice.h \
builders/residfp-builder/residfp/WaveformCalculator.cpp \
builders/residfp-builder/residfp/WaveformCalculator.h \
builders/residfp-builder/residfp/WaveformGenerator.cpp \
builders/residfp-builder/residfp/WaveformGenerator.h \
builders/residfp-builder/residfp/resample/Resampler.h \
builders/residfp-builder/residfp/resample/ZeroOrderResampler.h \
builders/residfp-builder/residfp/resample/SincResampler.cpp \
builders/residfp-builder/residfp/resample/SincResampler.h \
builders/residfp-builder/residfp/resample/TwoPassSincResampler.h \
builders/residfp-builder/residfp/version.cc

#=========================================================
# resid

BUILT_SOURCES = $(noinst_DATA:.dat=.h)

builders_resid_builder_resid_libresid_la_SOURCES = \
builders/resid-builder/resid/dac.h \
builders/resid-builder/resid/sid.h \
builders/resid-builder/resid/voice.h \
builders/resid-builder/resid/wave.h \
builders/resid-builder/resid/envelope.h \
builders/resid-builder/resid/filter.h \
builders/resid-builder/resid/extfilt.h \
builders/resid-builder/resid/pot.h \
builders/resid-builder/resid/spline.h \
builders/resid-builder/resid/dac.cc \
builders/resid-builder/resid/sid.cc \
builders/resid-builder/resid/voice.cc \
builders/resid-builder/resid/wave.cc \
builders/resid-builder/resid/envelope.cc \
builders/resid-builder/resid/filter.cc \
builders/resid-builder/resid/extfilt.cc \
builders/resid-builder/resid/pot.cc \
builders/resid-builder/resid/resid-config.h \
builders/resid-builder/resid/version.cc \
$(noinst_DATA:.dat=.h)

noinst_DATA = \
builders/resid-builder/resid/wave6581_PST.dat \
builders/resid-builder/resid/wave6581_PS_.dat \
builders/resid-builder/resid/wave6581_P_T.dat \
builders/resid-builder/resid/wave6581__ST.dat \
builders/resid-builder/resid/wave8580_PST.dat \
builders/resid-builder/resid/wave8580_PS_.dat \
builders/resid-builder/resid/wave8580_P_T.dat \
builders/resid-builder/resid/wave8580__ST.dat

noinst_SCRIPTS = builders/resid-builder/resid/samp2src.pl

#.dat.h:
#	$(PERL) $(srcdir)/samp2src.pl $* $< $(srcdir)/$@

#=========================================================
# builders
builders_residfp_builder_libsidplayfp_residfp_ladir = $(includedir)/sidplayfp/builders
builders_residfp_builder_libsidplayfp_residfp_la_HEADERS = \
builders/residfp-builder/residfp.h

builders_residfp_builder_libsidplayfp_residfp_la_SOURCES = \
builders/residfp-builder/residfp-builder.cpp \
builders/residfp-builder/residfp-emu.cpp \
builders/residfp-builder/residfp-emu.h

builders_residfp_builder_libsidplayfp_residfp_la_LIBADD = \
builders/residfp-builder/residfp/libresidfp.la


builders_resid_builder_libsidplayfp_resid_ladir = $(includedir)/sidplayfp/builders
builders_resid_builder_libsidplayfp_resid_la_HEADERS = \
builders/resid-builder/resid.h

builders_resid_builder_libsidplayfp_resid_la_SOURCES = \
builders/resid-builder/resid-builder.cpp \
builders/resid-builder/resid-emu.cpp \
builders/resid-builder/resid-emu.h

builders_resid_builder_libsidplayfp_resid_la_LIBADD = \
builders/resid-builder/resid/libresid.la

if HARDSID
builders_hardsid_builder_libsidplayfp_hardsid_ladir = $(includedir)/sidplayfp/builders
builders_hardsid_builder_libsidplayfp_hardsid_la_HEADERS = \
builders/hardsid-builder/hardsid.h

if MINGW32
  hardsid_src = builders/hardsid-builder/hardsid-emu-win.cpp
else
  hardsid_src = builders/hardsid-builder/hardsid-emu-unix.cpp
endif

builders_hardsid_builder_libsidplayfp_hardsid_la_SOURCES = \
$(hardsid_src) \
builders/hardsid-builder/hardsid-builder.cpp  \
builders/hardsid-builder/hardsid-emu.h
endif

#=========================================================
# libstilview
sidplayfp_libstilview_la_SOURCES = \
utils/STILview/stil.cpp

sidplayfp_libstilview_la_LDFLAGS = -version-info $(LIBSTILVIEWVERSION) $(W32_LDFLAGS)

sidplayfp_libstilview_ladir = $(includedir)/stilview

sidplayfp_libstilview_la_HEADERS = \
utils/STILview/stil.h \
utils/STILview/stildefs.h

#=========================================================
# docs
if HAVE_DOXYGEN
doc:
	$(DOXYGEN) Doxyfile

clean-local:
	rm -fr $(builddir)/docs/html
endif

#=========================================================
# test
if TESTSUITE
noinst_PROGRAMS = \
test/demo \
test/test \
builders/residfp-builder/residfp/resample/test

test_demo_SOURCES = test/demo.cpp 

test_demo_LDADD = sidplayfp/libsidplayfp.la

test_test_SOURCES = test/test.cpp 

test_test_LDADD = sidplayfp/libsidplayfp.la

builders_residfp_builder_residfp_resample_test_SOURCES = builders/residfp-builder/residfp/resample/test.cpp

builders_residfp_builder_residfp_resample_test_LDADD = builders/residfp-builder/residfp/resample/SincResampler.lo
endif

#=========================================================

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libsidplayfp.pc libstilview.pc

#=========================================================
# Recreate psiddrv.bin, needs xa65

CLEANFILES = sidplayfp/psiddrv.o65 sidplayfp/psiddrv.bin

sidplayfp/psiddrv.o65:
	xa -R -G $(srcdir)/sidplayfp/psiddrv.a65 -o $@

sidplayfp/psiddrv.bin: sidplayfp/psiddrv.o65
	od -v -An -w8 -tx1 $< | sed -re 's/[[:alnum:]]+/0x&,/g' > sidplayfp/psiddrv.bin
