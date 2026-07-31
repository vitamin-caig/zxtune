# Module (Player) Bounded Context

The playable abstraction of a recognized chiptune: holders, information, renderers, runtime state, and the track model. Sources: [`src/module`](/src/module).

## Aggregates & Entities

### Module::Holder (aggregate root)

- **Identity / Attributes**: Identified by `GetModuleInformation()` (duration, loop duration) and `GetModuleProperties()` (title, author, date, storage attrs, runtime attrs — see [`attributes.h`](/src/module/attributes.h)). Type via `ATTR_TYPE` (plugin id). Storage identity via `ATTR_CRC`/`ATTR_FIXEDCRC`.
- **Invariants**: A holder may create any number of independent `Renderer` instances. Renderers are created with a samplerate and parameters and are per-instance stateful ([`holder.h`](/src/module/holder.h#L36)).
- **Relationships**: produced by a `PlayerPlugin` (orchestration context). Consumed by `Service`, sound-output backends, and front-ends. May optionally implement `AdditionalFiles`.

### Renderer

- **Identity / Attributes**: Runtime playback engine bound to one module. `GetState()`, `Render()` (single frame), `Reset()`, `SetPosition(at)`.
- **Invariants**:
  - `Render()` returns an empty chunk when there is no more data.
  - Seeking out of range is safe. State stays `MODULE_PLAYING` until the next render call and produces only a flush ([`renderer.h`](/src/module/renderer.h#L39)).
- **Relationships**: created by `Holder`. Drives device-emulation event streams (via player-specific pipelines). Produces `Sound::Chunk`s consumed by the sound-output context.

### State / TrackState

- **Identity / Attributes**: Read-only playback snapshot: `At()` (position), `Total()` (played time ignoring seeks), `LoopCount()`. Track-specific: `Position`, `Pattern`, `Line`, `Tempo`, `Quirk`, `Channels` ([`track_state.h`](/src/module/track_state.h)).
- **Invariants**: `At()` ranges up to `Information::Duration`. `Position` up to `TrackInformation::PositionsCount`. `Channels` up to the channel count.
- **Relationships**: read model produced by `Renderer`. Consumed by sound backends and UI.

### Information / TrackInformation

- **Identity / Attributes**: `Duration()`, `LoopDuration()`; track variants add `ChannelsCount()`, `PositionsCount()`, `LoopPosition()` ([`track_information.h`](/src/module/track_information.h)).
- **Relationships**: immutable descriptor owned by `Holder`.

### TrackModel (aggregate: OrderList → PatternsSet → Pattern → Line → Cell/Command)

- **Identity / Attributes**: `OrderList` (`GetPatternIndex(pos)`, `GetLoopPosition()`). `PatternsSet`, `Pattern` (`GetLine(row)`, `GetSize()`). `Line` (`GetChannel(idx)`, `GetTempo()`). `Cell` (mask-gated note/sample/ornament/volume/commands). `Command` (type + 3 params) ([`track_model.h`](/src/module/players/track_model.h)).
- **Invariants**: Lines are built sequentially (`PatternBuilder::StartLine`). `MutablePatternsSet::GetSize()` counts only patterns with non-zero size. Sparse storage serves a stub for out-of-range indices ([`tracking.h`](/src/module/players/tracking.h)).
- **Relationships**: built by format decoders via `PatternsBuilder`. Consumed by `TrackStateIterator` and per-format players. `TrackModelState` combines the runtime position with the current `Pattern`/`Line` objects.

### AdditionalFiles

- **Identity / Attributes**: `Enumerate()` (expected sibling files). `Resolve(name, data)` (bind resolved data) ([`additional_files.h`](/src/module/additional_files.h)).
- **Invariants**: Resolution failures are surfaced to the caller as unknown data (see orchestration context).
- **Relationships**: optional capability of a `Holder`. Resolved by the orchestration context against archive contents.

### LoopParameters

- **Identity / Attributes**: `Enabled` + `Limit`. Predicate `(loopCount)`.
- **Invariants**: Domain convention: a single playback ends with `loopCount == 1`. The effective limit is the configured one plus one ([`loop.h`](/src/module/loop.h#L28)).
- **Relationships**: value object computed from `zxtune.sound.looped` / `zxtune.sound.looplimit`. Honored by the pipelined renderer.

### PipelinedRenderer

- **Identity / Attributes**: Wrapper renderer applying loop control, gain, fadein/fadeout, and silence detection ([`pipeline.h`](/src/module/players/pipeline.h)).
- **Invariants**: Samplerate comes from global params. Other properties fall back from holder properties to global params in specified order.
- **Relationships**: created around a `Holder`. Links the player context to sound-output parameters.

## Related contexts

- [Orchestration](orchestration.md) — detection/open flows produce `Module::Holder`s. Handles additional-files resolution.
- [Format Recognition](format-recognition.md) — decoded chiptune containers feed per-format players.
- [Device Emulation](device-emulation.md) — players translate track/stream events into device register writes (`DataChunk`s).
- [Sound Output](sound-output.md) — renderers produce `Sound::Chunk`s. The pipelined renderer consumes `Sound::RenderParameters` and module state.
- [Front-ends](frontends.md) — CLI/Qt/Android create backends from holders and display `State`/`Information`.

## Open questions

- Loop-count off-by-one convention (`loopCount == 1` for single playback) is only stated in a code comment. Not formalized for consumers computing fadeout timing.
- Seek "flush-only" semantics: exact audible behavior of out-of-range seeks is implementation-specific.
- `ATTR_CHANNELS_NAMES` should only be set when `zxtune.core.channels_mask` is supported. The guard is implicit ([`attributes.h`](/src/module/attributes.h#L52)).
- `Module::State::Total()` semantics ("ignoring seeks") vs. `At()` interplay is not fully specified.
