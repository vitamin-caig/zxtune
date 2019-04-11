library_name := xmp
path_step := ../..
include $(path_step)/make/default.mak

includes.dirs += include src

source_files += $(addprefix src/,\
 virtual format misc period player read_event \
 dataio mkstemp fnmatch md5 lfo envelope scan \
 control med_extras filter effects mixer \
 mix_all \
 load_helpers load hio hmn_extras extras smix)

source_files += $(addprefix src/depackers/,\
  readlzw readrle)

# ppdepack unsqsh mmcmp readhuff \
# unarc arcfs xfd muse unlzx s404_dec \
# unzip gunzip uncompress unxz bunzip2 unlha \
# xz_dec_lzma2 xz_dec_stream oxm vorbis crc32)

source_files += $(addprefix src/loaders/,\
 common iff itsex asif voltable sample \
 xm_load mod_load stm_load 669_load far_load \
 mtm_load ptm_load okt_load \
 ult_load mdl_load it_load stx_load pt3_load sfx_load \
 flt_load st_load emod_load imf_load digi_load fnk_load \
 ice_load liq_load ims_load masi_load amf_load \
 psm_load s3m_load stim_load mmd_common mmd1_load mmd3_load \
 rtm_load dmf_load tcb_load dt_load gtk_load no_load \
 arch_load sym_load dtt_load mgt_load med2_load med3_load \
 med4_load ssmt_load dbm_load umx_load gdm_load \
 coco_load gal5_load gal4_load \
 polly_load asylum_load hmn_load)

# pw_load

#source_files += $(addprefix src/loaders/prowizard/,\
# prowiz ptktable tuning ac1d di eureka \
# fc-m fuchs fuzzac gmc heatseek kris ksm \
# mp np1 np2 np3 p61a pm10c pm18a \
# pha prun1 prun2 tdd unic unic2 wn zen \
# tp3 p40 xann theplayer pp21 starpack titanics \
# skyt novotrade hrt noiserun)

#
# pm pm01 \
# pm20 pm40 pp10 pp30 tp1 tp2)

ifneq ($(findstring $(platform),windows mingw),)
source_files += $(addprefix src/win32/,\
 ptpopen debug usleep)

includes.dirs += src/win32
endif

source_files := $(addsuffix .c,$(source_files))

defines += BUILDING_STATIC NO_VERSIONED_SYMBOLS DECRUNCH_MAX=0 NO_PROWIZARD NO_EXTERNALFILES PATH_MAX=1024
defines.windows += inline=__inline _USE_MATH_DEFINES WIN32 _WINDOWS

include $(path_step)/makefile.mak
