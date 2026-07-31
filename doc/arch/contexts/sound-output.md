# Sound Rendering & Output Bounded Context

Mixing, post-processing, and delivery of rendered audio to OS/device backends. Sources: [`src/sound`](/src/sound).

## Aggregates & Entities

### Backend (aggregate root)

- **Identity / Attributes**: `BackendInformation`: `Id()` (short spaceless identifier), `Description()`, `Capabilities()`, `Status()` (actuality error) ([`backend.h`](/src/sound/backend.h#L23)).
- **Invariants**: Bound to a single `Module::Holder` at creation via `Sound::Service::CreateBackend(id, module, callback)`. Exposes `GetState()`, `GetAnalyzer()`, `GetPlaybackControl()`, optional `GetVolumeControl()` ([`service.h`](/src/sound/service.h#L38)).
- **Relationships**: implemented per sink (system/file/OS audio in [`src/sound/backends`](/src/sound/backends)). Created by `Sound::Service` factories (`CreateSystemService`, `CreateFileService`, `CreateGlobalService`).

### PlaybackControl

- **Identity / Attributes**: `Play()`, `Pause()`, `Stop()`, `SetPosition(frame)`, `GetCurrentState()` (`STOPPED` / `PAUSED` / `STARTED`).
- **Invariants**: Idempotent transitions — no effect if already in the requested state. Out-of-range seek stops playback ([`backend.h`](/src/sound/backend.h#L62)).
- **Relationships**: owned by a `Backend`. Driven by front-ends (transport controls).

### VolumeControl

- **Identity / Attributes**: `GetVolume()` / `SetVolume(Gain)` for the hardware mixer.
- **Invariants**: May be unsupported (backend returns empty pointer). `Gain` is normalized to `[0..1]` per channel ([`gain.h`](/src/sound/gain.h#L40)).
- **Relationships**: optional capability of a `Backend`.

### BackendCallback

- **Identity / Attributes**: Observes playback lifecycle: `OnStart`, `OnFrame(state)`, `OnStop`, `OnPause`, `OnResume`, `OnFinish` ([`backend.h`](/src/sound/backend.h#L129)).
- **Relationships**: supplied by the caller at backend creation. Notified with `Module::State` snapshots.

### Analyzer

- **Identity / Attributes**: `GetSpectrum(LevelType*, limit)` — fixed-point level array (`FixedPoint<uint8_t, 100>`).
- **Relationships**: owned by a `Backend`. Consumed by visualizers.

### RenderParameters

- **Identity / Attributes**: `Version()`, `SoundFreq()`. Built from sound parameters ([`render_params.h`](/src/sound/render_params.h)).
- **Invariants**: `SoundFreq()` defaults to 44.1 kHz (`zxtune.sound.frequency`).
- **Relationships**: input to the player context's pipelined renderer. `GetLoopParameters`/`GetSoundFrequency` read `zxtune.sound.*` (looped, looplimit, fadein, fadeout, gain, silencelimit — [`sound_parameters.h`](/src/sound/sound_parameters.h)).

### Sound::Service

- **Identity / Attributes**: `EnumerateBackends()`, `GetAvailableBackends()` (preference-ordered ids), `CreateBackend(id, module, callback)` ([`service.h`](/src/sound/service.h)).
- **Relationships**: orchestrates backend selection for front-ends. Produced by system/file/global factory variants.

## Value objects

- **`Sound::Sample`** — fixed 16-bit stereo (`MIN/MID/MAX`, fast-add for mixing) ([`sample.h`](/src/sound/sample.h)).
- **`Sound::Chunk`** — move-only block of samples. Renderer output, receiver input ([`chunk.h`](/src/sound/chunk.h)).
- **`Sound::Gain`** — fixed-point per-channel volume with `IsNormalized()` ([`gain.h`](/src/sound/gain.h)).
- **`MultichannelSample<N>` / mixers / receivers** — typed channel-count adapters (`OneChannelMixer`..`FourChannelsMixer`, `FixedChannelsReceiver<N>`) ([`mixer.h`](/src/sound/mixer.h), [`receiver.h`](/src/sound/receiver.h)).

## Related contexts

- [Player](player.md) — renders `Module::Holder` into `Chunk`s. The pipelined renderer consumes `RenderParameters` and module `State`.
- [Device Emulation](device-emulation.md) — chips produce `Chunk`s through the mixers used here.
- [Orchestration](orchestration.md) — supplies parameters (`zxtune.sound.*`) and creates `Sound::Service` for front-ends.
- [Front-ends](frontends.md) — front-ends create backends from holders, drive `PlaybackControl`, display `State`/spectrum.

## Open questions

- `VolumeControl` doc comments state "throw Error in case of success" — the error-on-success phrasing appears self-contradictory and needs clarification ([`backend.h`](/src/sound/backend.h#L52)).
- `Analyzer::GetSpectrum` does not specify the number of bands, FFT windowing, or exact meaning of `limit` vs. actual bands.
- Backend capabilities bitmask semantics are declared in `backend_attrs.h`. Per-backend guarantees are not enumerated here.
