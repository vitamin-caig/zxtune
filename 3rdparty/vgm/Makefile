library_name := vgm
dirs.root := ../..
include $(dirs.root)/make/default.mak

source_files = $(addprefix emu/,SoundEmu Resampler panning dac_control) \
               $(addprefix emu/cores/, \
                sn764intf sn76496 sn76489 \
                2413intf emu2413 ym2413 \
                nukedopll nukedopl \
                2612intf fmopn2612 ym2612 \
                ym3438 \
                ym2151 \
                segapcm \
                rf5cintf rf5c68 \
                scd_pcm \
                opnintf \
                fmopn \
                ymdeltat \
                oplintf \
                fmopl \
                adlibemu_opl2 adlibemu_opl3 \
                262intf ymf262 \
                ymf278b \
                ymf271 \
                ymz280b \
                ayintf ay8910 emu2149 \
                pwm \
                gb \
                nesintf nes_apu np_nes_apu np_nes_dmc np_nes_fds \
                multipcm \
                upd7759 \
                okim6258 okim6295 okiadpcm \
                k051649 \
                k054539 \
                c6280intf c6280_mame \
                Ootake_PSG \
                c140 \
                c219 \
                k053260 \
                pokey \
                qsoundintf qsound_mame qsound_ctr \
                scsp scspdsp \
                ws_audio \
                vsu \
                saaintf saa1099_mame saa1099_vb \
                es5503 es5506 \
                x1_010 \
                c352 \
                iremga20 \
        )

source_files += $(addprefix utils/,DataLoader StrUtils-CPConv_Stub)

source_files := $(addsuffix .c,$(source_files))

source_files += $(addprefix player/,playerbase.cpp vgmplayer.cpp vgmplayer_cmdhandler.cpp helper.c dblk_compr.c)

include $(dirs.root)/makefile.mak
