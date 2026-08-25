library_name := module
dirs.root := ../..
source_dirs := players players/aym players/dac players/external players/external/sid players/external/wav players/multitrack players/saa players/tfm players/xsf

po_files := module

includes.dirs.gsf.cpp = $(dirs.root)/3rdparty/mgba/include $(dirs.root)/3rdparty/mgba/src
includes.files.gsf.cpp = $(dirs.root)/3rdparty/mgba/defines.h
includes.dirs.vgmstream.cpp = $(dirs.root)/3rdparty/vgmstream/ext_includes
includes.dirs.sid.cpp = $(dirs.root)/3rdparty/sidplayfp/src
includes.dirs.ffmpeg.cpp = $(dirs.root)/3rdparty/ffmpeg
includes.dirs.ogg.cpp = $(dirs.root)/3rdparty

defines.gme.cpp = HAVE_STDINT_H
defines.openmpt.cpp = BUILDING_STATIC
defines.xmp.cpp = BUILDING_STATIC
defines.flac.cpp = FLAC__NO_DLL
defines.mp3.cpp = MINIMP3_IMPLEMENTATION MINIMP3_NONSTANDARD_BUT_LOGICAL
defines.ogg.cpp = OV_EXCLUDE_STATIC_CALLBACKS

jumbo.name := $(library_name)
jumbo.excludes := $(addprefix players/, \
                   $(addprefix external/, asap.cpp flac.cpp gme.cpp libvgm.cpp mp3.cpp ogg.cpp openmpt.cpp sid.cpp spc.cpp vgmstream.cpp wav/ffmpeg.cpp xmp.cpp) \
                   dac/abysshighestexperience.cpp \
                   $(addprefix xsf/, 2sf.cpp gsf.cpp ncsf.cpp psf.cpp sdsf.cpp usf.cpp) \
                  )

include $(dirs.root)/makefile.mak

ifdef tools.python
players/external/sid/songlengths_db.inc: players/external/sid/mksonglength.py $(sort $(wildcard players/external/sid/Songlengths*.md5))
	$(tools.python) $^ > $@
endif

players/external/sid/songlengths.cpp: players/external/sid/songlengths_db.inc
