library_name := lazyusf2
path_step := ../..

source_files += ai/ai_controller \
                api/callbacks \
                $(addprefix main/, main rom savestates util) \
                memory/memory_impl \
                $(addprefix pi/, cart_rom pi_controller) \
                $(addprefix r4300/, cached_interp cp0 cp1 exception interupt mi_controller pure_interp r4300 r4300_core recomp reset tlb) \
                rdp/rdp_core \
                $(addprefix ri/, rdram rdram_detection_hack ri_controller) \
                rsp/rsp_core \
                $(addprefix rsp_hle/, alist alist_audio alist_naudio alist_nead audio cicx105 hle jpeg memory mp3 musyx plugin) \
                rsp_lle/rsp \
                $(addprefix si/, cic game_controller n64_cic_nus_6105 pif si_controller) \
                $(addprefix usf/, usf resampler) \
                vi/vi_controller

source_files := $(addsuffix .c,$(source_files))

include_dirs := .
windows_definitions += WIN32

include $(path_step)/makefile.mak
