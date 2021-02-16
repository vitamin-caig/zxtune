library_name := xmp
dirs.root := ../..
include $(dirs.root)/make/default.mak

includes.dirs += include src

source_files += $(addprefix src/,\
  control dataio effects envelope extras filter fnmatch format hio hmn_extras lfo \
  load load_helpers md5 med_extras misc mixer mix_all period player read_event scan smix virtual \
  )

source_files += $(addprefix src/loaders/,\
 common iff sample mmd_common \
 dtt_load emod_load fnk_load gtk_load ims_load liq_load med2_load med3_load med4_load no_load rtm_load stim_load stx_load tcb_load\
 )

source_files := $(addsuffix .c,$(source_files))

defines += BUILDING_STATIC NO_VERSIONED_SYMBOLS DECRUNCH_MAX=0 NO_PROWIZARD NO_EXTERNALFILES NO_COMPOSITE_LOADER PATH_MAX=1024
defines.windows += inline=__inline _USE_MATH_DEFINES WIN32 _WINDOWS

include $(dirs.root)/makefile.mak
