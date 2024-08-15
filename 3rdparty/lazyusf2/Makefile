library_name := lazyusf2
dirs.root := ../..

source_files += ai/ai_controller \
                api/callbacks \
                $(addprefix main/, main rom savestates util) \
                memory/memory \
                $(addprefix pi/, cart_rom pi_controller) \
                $(addprefix r4300/, cached_interp cp0 cp1 exception interupt mi_controller r4300 r4300_core recomp reset tlb pure_interp) \
                rdp/rdp_core \
                $(addprefix ri/, rdram rdram_detection_hack ri_controller) \
                rsp/rsp_core \
                $(addprefix rsp_hle/, alist alist_audio alist_naudio alist_nead audio cicx105 hle hvqm jpeg memory mp3 musyx plugin re2) \
                rsp_lle/rsp \
                $(addprefix si/, cic game_controller n64_cic_nus_6105 pif si_controller) \
                $(addprefix usf/, usf resampler barray) \
                vi/vi_controller

includes.dirs += . $(dirs.root)/3rdparty/zlib

ifdef lazyusf2.debug
defines += DEBUG_INFO
source_files += debugger/dbg_decoder
endif

source_files := $(addsuffix .c,$(source_files))

objects_flat_names := 1

include $(dirs.root)/makefile.mak
