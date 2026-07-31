# Device Emulation Bounded Context

Chip-accurate emulation of vintage sound hardware and register-write dumpers. Sources: [`src/devices`](/src/devices) (`aym`, `fm`, `saa`, `dac`, `beeper`, `turbosound`, `z80`).

## Aggregates & Entities

### Device (aggregate root, per chip family: AYM, FM, SAA, DAC, Beeper, Turbosound)

- **Identity / Attributes**: Chip type plus `ChipParameters`: `ClockFreq()`, `SoundFreq()`, `Type()`, `Interpolation()`, `DutyCycleValue()`, `DutyCycleMask()`, `Layout()`, `MuteMask()`, `Version()` (see [`aym/chip.h`](/src/devices/aym/chip.h#L63)).
- **Invariants**:
  - State is purely a function of submitted chunks and `Reset()`. The device renders synchronously to a requested timestamp (`RenderTill`) ([`aym/chip.h`](/src/devices/aym/chip.h#L25)).
  - Register writes are collected in `DataChunk` batches with timestamps. `Registers` track a write mask, so only touched registers are emitted ([`aym.h`](/src/devices/aym.h#L29)).
- **Relationships**: fed by the player context's event streams (`DataChunk` sequences). Mixes chip channels (AY: `SOUND_CHANNELS = 3`) into `Sound::Chunk` via a `MixerType`. `ChipParameters` is a value object sourced from `zxtune.core.*` parameters (orchestration context).

### Chip

- **Identity / Attributes**: The concrete emulator implementing `Device`. Adds `RenderTill(Stamp)` producing `Sound::Chunk`.
- **Invariants**: Advances internal state exactly to the requested stamp.
- **Relationships**: created via virtual factories (e.g. `Devices::AYM::CreateChip(params, mixer)`). Output is consumed by the sound-output context.

### Dumper

- **Identity / Attributes**: Device variant that records register writes instead of synthesizing audio. `GetDump()` yields the recorded data. Families: PSG, ZX50, FYM, Debug, RawStream ([`aym/dumper.h`](/src/devices/aym/dumper.h)).
- **Invariants**: `DumperParameters::FrameDuration` and `Optimization` level shape the recorded stream. FYM dumps additionally require `ClockFreq`, `Title`, `Author`, `LoopFrame` ([`aym/dumper.h`](/src/devices/aym/dumper.h#L53)).
- **Relationships**: produced dump data feeds back into format-recognition containers (conversion plugins: OUT, PSG, YM, ZX50, TXT, AYDUMP, FYM).

### ChipParameters (value object)

- **Identity / Attributes**: `Version`, `ClockFreq`, `SoundFreq`, `Type`, `Interpolation`, `DutyCycleValue`, `DutyCycleMask`, `Layout`, `MuteMask`.
- **Relationships**: constructed from parameters (`zxtune.core.aym.*`, `zxtune.core.fm.*`, `zxtune.core.saa.*`, `zxtune.core.sid.*`, ...) by the orchestration/player wiring. Channel masks (`A B C N E`) map to `ATTR_CHANNELS_NAMES` in the player context.

### Z80

- **Identity / Attributes**: CPU emulator used by emulation-format players (memory-dump modules). `INT_TICKS`, `CLOCKRATE` parameters (`zxtune.core.z80.*`) ([`core_parameters.h`](/src/core/core_parameters.h#L128)).
- **Relationships**: supports the player context for `MEMORYDUMP` module type. Feeds chips via the emulated CPU's register/port writes.

## Related contexts

- [Player](player.md) — produces the `DataChunk` event streams. Consumes dumpers for conversion output.
- [Sound Output](sound-output.md) — chips render `Sound::Chunk`s through mixers. Interpolation/quality parameters originate from shared parameters.
- [Orchestration](orchestration.md) — supplies `zxtune.core.*` parameters (clockrate, chip type, layout, mute mask) that shape `ChipParameters`.
- [Format Recognition](format-recognition.md) — dumped streams are encoded into interchange containers.

## Open questions

- Emulation fidelity contracts (interpolation LQ/HQ vs none, duty-cycle precision) are parameter-driven. Audible equivalence across chips is not specified.
- `ChipParameters::Version()` semantics and its invalidation contract with renderers are not documented.
- Mute masks (`MuteMask`) interact with `ATTR_CHANNELS_NAMES`. The exact precedence between parameter muting and track-level channel disabling is implicit.
