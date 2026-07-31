# Front-ends Bounded Context

End-user and tooling applications built on the core libraries. Sources: [`apps/`](/apps) (`zxtune123`, `zxtune-qt`, `zxtune-android`, `xtractor`, `benchmark`, `tools/`).

## Aggregates & Entities

### SourceComponent (CLI)

- **Identity / Attributes**: Command-line data source provider: `GetOptionsDescription()`, `ParseParameters()`, `Initialize()`, `ProcessItems(callback)` ([`source.h`](/apps/zxtune123/source.h)).
- **Invariants**: `Initialize()` may throw. Processing is cancelled via `CancelError` ([`source.h`](/apps/zxtune123/source.h#L28)).
- **Relationships**: produces `OnItemCallback::ProcessItem(data, holder)` per recognized module, or `ProcessUnknownData(path, container, data)` otherwise. Created from config parameters. Consumes the orchestration `Service` and module holders.

### SoundComponent (CLI)

- **Identity / Attributes**: Audio subsystem wrapper: `GetOptionsDescription()`, `ParseParameters()`, `Initialize()`, `CreateBackend(module, typeHint, callback)`, `EnumerateBackends()`, `GetSamplerate()` ([`sound.h`](/apps/zxtune123/sound.h)).
- **Relationships**: wraps the sound-output `Sound::Service`. Binds a `Module::Holder` to a `Sound::Backend` for the CLI player loop.

### OnItemCallback

- **Identity / Attributes**: `ProcessItem(data, holder)`, `ProcessUnknownData(path, container, data)` ([`source.h`](/apps/zxtune123/source.h#L31)).
- **Relationships**: callback contract between `SourceComponent` and the CLI playback/display logic.

### XTractor Analysis::Node

- **Identity / Attributes**: `Name()`, `Data()` (never empty), `Parent()`. Root/subnode factories ([`main.cpp`](/apps/xtractor/main.cpp#L59)).
- **Invariants**: Non-root nodes have a parent. Associated data is always present.
- **Relationships**: tree model over scanner results. Lets users extract embedded modules from arbitrary dumps.

### zxtune-qt application

- **Identity / Attributes**: Qt desktop UI (`qt_app.cpp`), single-instance mode (`singlemode.cpp`), playlist model ([`apps/zxtune-qt/playlist`](/apps/zxtune-qt/playlist)), online/bundle support.
- **Relationships**: drives `Sound::Backend` transport, playlist of modules, spectrum via `Sound::Analyzer`.

### zxtune-android application

- **Identity / Attributes**: Mobile player with Now Playing UI, playlist/browser tabs, analytics, ringtone export ([`apps/zxtune-android`](/apps/zxtune-android)).
- **Relationships**: same core services. Consumes `Module::Information`/`State` and sound backends.

## Related contexts

- [Orchestration](orchestration.md) — front-ends call `ZXTune::Service` open/detect flows.
- [Sound Output](sound-output.md) — backends and transport controls are created/bound by front-ends.
- [Player](player.md) — holders/state/information drive playback and UI metadata.
- [Extraction](extraction.md) — xtractor builds node trees over scanner results. CLI resolves subpaths via `Analysis::Path`.

## Open questions

- Cross-platform feature parity between front-ends (e.g. channels muting, conversion export) is not centrally documented.
- `OnItemCallback::ProcessUnknownData` default is a no-op. Policy on surfacing unmatched data is per-app.
