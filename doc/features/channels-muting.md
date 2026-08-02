# Channels muting functionality reference

`ATTR_CHANNELS_NAMES` is a newline-delimited string property set on modules that support per-channel muting (via `zxtune.core.channels_mask` integer property, bitmask).
Set up via [`PropertiesHelper::SetChannels()`](/src/module/players/properties_helper.cpp#L108).

IMPORTANT: for newly implemented low-level support (e.g. at 3rdparty libraries) channels muting affects only final rendered sound, internal state update sequence SHOULD stay intact.

---

## Fixed / formula-based sources

| Format | Source | Channel names |
|---|---|---|
| AYM single-chip (PT1, PT2, PT3, ASC, STC, STP, ST1, ST3, FTC, GTR, PSC, PSM, SQT, PSG, VTX, AYC, AY…) | [`aym_properties_helper.cpp`](/src/module/players/aym/aym_properties_helper.cpp#L27) | `A` `B` `C` `N` `E` |
| E-Tracker (SAM Coupé) | [`etracker.cpp`](/src/module/players/saa/etracker.cpp#L485) | `A/1` `B/1` `C/1` `N/1` `E/1` `A/2` `B/2` `C/2` `N/2` `E/2` |
| GSF (GBA) | [`gsf.cpp`](/src/module/players/xsf/gsf.cpp#L387) | `PU1` `PU2` `WAV` `NZE` `A` `B` |
| TurboSound / PT3-TS | [`protracker3.cpp`](/src/module/players/aym/protracker3.cpp#L493) | `A/1` `B/1` `C/1` `N/1` `E/1` `A/2` `B/2` `C/2` `N/2` `E/2` |
| 2SF / NCSF (Nintendo DS) | [`2sf.cpp`](/src/module/players/xsf/2sf.cpp#L314), [`ncsf.cpp`](/src/module/players/xsf/ncsf.cpp#L274) | `DSP 1`…`DSP 16` |
| PSF (PS1) | [`psf.cpp`](/src/module/players/xsf/psf.cpp#L393) | `SPU 1`…`SPU 24` |
| PSF2 (PS2) | [`psf.cpp`](/src/module/players/xsf/psf.cpp#L399) | `SPU2 1`…`SPU2 48` |
| SDSF (Saturn) | [`sdsf.cpp`](/src/module/players/xsf/sdsf.cpp#L288) | `SCSP 1`…`SCSP 32` |
| DSF (Dreamcast) | [`sdsf.cpp`](/src/module/players/xsf/sdsf.cpp#L294) | `AICA 1`…`AICA 64` |
| SPC (SNES) | [`spc_supp.cpp`](/src/core/plugins/players/gme/spc_supp.cpp#L288) | `DSP 1`…`DSP 8` |
| SID (C64), 1 chip | [`sid_base.cpp`](/src/core/plugins/players/sid/sid_base.cpp#L364) | `Voice 1`…`Voice 3` |
| SID (C64), 2 chips | [`sid_base.cpp`](/src/core/plugins/players/sid/sid_base.cpp#L364) | `Voice 1/1`…`Voice 3/1` `Voice 1/2`…`Voice 3/2` |
| ASAP (Atari), 1 chip | [`asap_base.cpp`](/src/core/plugins/players/asap/asap_base.cpp#L166) | `C1`…`C4` |
| ASAP (Atari), 2 chips | [`asap_base.cpp`](/src/core/plugins/players/asap/asap_base.cpp#L166) | `L1`…`L4` `R1`…`R4` |

---

## OpenMPT-based sources

Channel names are set only when the module supports the `openmpt::ext::interactive` interface (required for muting). The logic in [`FillMetadata()`](/src/core/plugins/players/mpt/openmpt_base.cpp#L395):

1. `module.get_channel_names()` — returns per-channel names stored in the file (may be empty strings).
2. For each channel where the name is empty, the fallback `<PluginId> <index>` is used (e.g. `XM 1`, `IT 4`) — 1-based index.
3. If the `interactive` interface is not available, `ATTR_CHANNELS_NAMES` is **not set** at all.

---

## GME-based sources (via `emu->voice_name(i)`)

| Format | Emulator | Channel names |
|---|---|---|
| GBS (Game Boy) | [`Gbs_Emu.cpp`](/3rdparty/gme/gme/Gbs_Emu.cpp#L121) | `Square 1` `Square 2` `Wave` `Noise` |
| NSF/NSFE (NES) base | [`Nsf_Emu.cpp`](/3rdparty/gme/gme/Nsf_Emu.cpp#L156) | `Square 1` `Square 2` `Triangle` `Noise` `DMC` |
| NSF + VRC6 | [`Nsf_Emu.cpp`](/3rdparty/gme/gme/Nsf_Emu.cpp#L178) | + `Square 3` `Square 4` `Saw Wave` |
| NSF + FME7 | [`Nsf_Emu.cpp`](/3rdparty/gme/gme/Nsf_Emu.cpp#L191) | + `Square 3` `Square 4` `Square 5` |
| NSF + MMC5 | [`Nsf_Emu.cpp`](/3rdparty/gme/gme/Nsf_Emu.cpp#L204) | + `Square 3` `Square 4` `PCM` |
| NSF + FDS | [`Nsf_Emu.cpp`](/3rdparty/gme/gme/Nsf_Emu.cpp#L217) | + `FM` |
| NSF + Namco | [`Nsf_Emu.cpp`](/3rdparty/gme/gme/Nsf_Emu.cpp#L230) | + `Wave 1`…`Wave 8` |
| NSF + VRC7 | [`Nsf_Emu.cpp`](/3rdparty/gme/gme/Nsf_Emu.cpp#L245) | + `FM 1`…`FM 6` |
| HES (PC-Engine) | [`Hes_Emu.cpp`](/3rdparty/gme/gme/Hes_Emu.cpp#L144) | `Wave 1`…`Wave 4` `Multi 1` `Multi 2` `ADPCM` |
| GYM (Sega Genesis) | [`Gym_Emu.cpp`](/3rdparty/gme/gme/Gym_Emu.cpp#L234) | `FM 1`…`FM 6` `PCM` `PSG` |
| KSS/KSSX (SMS) | [`Kss_Emu.cpp`](/3rdparty/gme/gme/Kss_Emu.cpp#L202) | `Square 1`…`Square 3` `Noise` [+ `FM`] |
| KSS/KSSX (MSX) | [`Kss_Emu.cpp`](/3rdparty/gme/gme/Kss_Emu.cpp#L228) | `Square 1`…`Square 3` [+ `FM`] |
| KSS/KSSX (MSX+SCC) | [`Kss_Emu.cpp`](/3rdparty/gme/gme/Kss_Emu.cpp#L271) | `Square 1`…`Square 3` `Wave 1`…`Wave 5` |

---

## VGM/S98 sources (via libvgm `Device::ChannelsNames()`)

Name composing logic in [`ChannelsLayout::GetChannelsNames()`](/src/core/plugins/players/vgm/vgm_base.cpp#L181):

- **Single-channel device, one instance**: bare `<DevName>` (e.g. `32X PWM`)
- **Multi-channel device, one instance, named channels**: `<DevName> <chName>` (e.g. `YM2612 FM 1`)
- **Multi-channel device, one instance, numeric fallback**: `<DevName> <ch+1>` (e.g. `SAA1099 1`, `SAA1099 6`)
- **Multiple instances of same chip type**: instance suffix `/<instance+1>` appended after the channel name/number: `<DevName> <chName>/<instance+1>` (e.g. `YM2612 FM 1/1`, `YM2612 FM 1/2`)

Empty array (channels not set) if total channel count across all devices exceeds 63.

### Devices with named channels

| Device name | Source | Channel names |
|---|---|---|
| `YM2612` / `YM3438` | [`2612intf.c`](/3rdparty/vgm/emu/cores/2612intf.c#L127) | `FM 1`…`FM 6` `DAC` |
| `YM2608` | [`opnintf.c`](/3rdparty/vgm/emu/cores/opnintf.c#L158) | `FM 1`…`FM 6` `ADPCM-A 1`…`ADPCM-A 6` `ADPCM-B` |
| `YM2610` / `YM2610B` | [`opnintf.c`](/3rdparty/vgm/emu/cores/opnintf.c#L247) | `FM 1`…`FM 6` `ADPCM-A 1`…`ADPCM-A 6` `ADPCM-B` |
| `YMF262` | [`262intf.c`](/3rdparty/vgm/emu/cores/262intf.c#L131) | `1`…`18` `Bass Drum` `Snare Drum` `Tom Tom` `Cymbal` `Hi-Hat` |
| `YM2413` / `VRC7` | [`2413intf.c`](/3rdparty/vgm/emu/cores/2413intf.c#L34) | `1`…`9` `Bass Drum` `Snare Drum` `Tom Tom` `Cymbal` `Hi-Hat` |
| `YM3812` / `YM3526` | [`oplintf.c`](/3rdparty/vgm/emu/cores/oplintf.c#L130) | `1`…`9` `Bass Drum` `Snare Drum` `Tom Tom` `Cymbal` `Hi-Hat` |
| `Y8950` | [`oplintf.c`](/3rdparty/vgm/emu/cores/oplintf.c#L262) | `1`…`9` `Bass Drum` `Snare Drum` `Tom Tom` `Cymbal` `Hi-Hat` `ADPCM` |
| `GameBoy DMG` | [`gb.c`](/3rdparty/vgm/emu/cores/gb.c#L126) | `Square 1` `Square 2` `Wave` `Noise` |
| `NES APU` / `NES APU + FDS` | [`nesintf.c`](/3rdparty/vgm/emu/cores/nesintf.c#L142) | `Square 1` `Square 2` `Triangle` `Noise` `DPCM` `FDS` |
| `SN76496` / `SEGA PSG` / `T6W28` / etc. | [`sn764intf.c`](/3rdparty/vgm/emu/cores/sn764intf.c#L59) | `1` `2` `3` `Noise` |
| `QSound` | [`qsoundintf.c`](/3rdparty/vgm/emu/cores/qsoundintf.c#L29) | `PCM 1`…`PCM 16` `ADPCM 1`…`ADPCM 3` |
| `BSMT2000` | [`bsmt2000.c`](/3rdparty/vgm/emu/cores/bsmt2000.c#L186) | `PCM 1`…`PCM 12` `ADPCM` |

### Devices with numeric fallback (`channelNames` returns NULL → `<DevName> <ch+1>`)

All devices not listed in the named-channels table above. Confirmed NULL-returning devices include:

`YM2203`, `YM2151`, `AY-3-8910`/`YM2149`/`YM3439`/`YMZ284`/`YMZ294`/`AY8930`/etc., `C6280`, `SAA1099`, `RF5C68`/`RF5C164`/`RF5C105`, `ES5503`, `C352`, `C219`, `C140`, `K051649`/`K052539`, `K005289`, `K007232`, `K053260`, `K054539`, `YMW258` (MultiPCM), `MSM6258`, `MSM6295`, `Pokey`, `SCSP`, `Sega PCM`, `32X PWM`, `uPD7759`, `VBoy VSU`, `X1-010`, `WonderSwan`, `YMF278B`, `YMF271`, `YMZ280B`, `GA20`, `ICS2115`, `Mikey`, `MSM5205`/`MSM6585`, `MSM5232`.

---

## TFM-based formats

| Format | Source | Channel names |
|---|---|---|
| TFC, TFD, TFE (TFMMusicMaker) | [`tfc.cpp`](/src/module/players/tfm/tfc.cpp#L239), [`tfd.cpp`](/src/module/players/tfm/tfd.cpp#L137), [`tfmmusicmaker.cpp`](/src/module/players/tfm/tfmmusicmaker.cpp#L1516) | `FM 1`…`FM 6` |

---

## Other formats with dynamic channel names

| Format | Source | Channel names |
|---|---|---|
| AHX / HVL | [`abysshighestexperience.cpp`](/src/module/players/dac/abysshighestexperience.cpp#L336) | `A`…`P` |
| V2M | [`v2m.cpp`](/src/module/players/dac/v2m.cpp#L286) | `MIDI 1`…`MIDI 15` `Ronan` |
| XMP (DTT, EMOD, FNK, LIQ, MED, STIM, STX) | [`xmp_base.cpp`](/src/core/plugins/players/xmp/xmp_base.cpp#L418) | `<PluginId> 1`…`<PluginId> N` (e.g. `DTT 1`, `EMOD 2`) |

Most other DAC formats use [`DAC::PropertiesHelper`](/src/module/players/dac/dac_properties_helper.cpp#L19) which sets channels automatically.

---

## Plugins that do NOT set `ATTR_CHANNELS_NAMES`

### XSF

- **USF** (Nintendo 64) — [`usf.cpp`](/src/module/players/xsf/usf.cpp#L259) sets only platform, no channels

### Third-party engine wrappers

- **vgmstream** — [`vgmstream_base.cpp`](/src/core/plugins/players/vgmstream/vgmstream_base.cpp#L445)

### Music / raw audio formats

[`wav_supp.cpp`](/src/core/plugins/players/music/wav_supp.cpp), [`ogg_supp.cpp`](/src/core/plugins/players/music/ogg_supp.cpp), [`mp3_supp.cpp`](/src/core/plugins/players/music/mp3_supp.cpp), [`flac_supp.cpp`](/src/core/plugins/players/music/flac_supp.cpp):

- **WAV**, **OGG**, **MP3**, **FLAC**

### Multi

- **MTC** — [`mtc_supp.cpp`](/src/core/plugins/players/multi/mtc_supp.cpp#L344)
