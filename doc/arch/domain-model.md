# ZXTune Domain Model

Domain analysis of ZXTune — an open-source crossplatform chiptunes player.

## Overview

ZXTune plays music in variety of formats, chiptunes primarily. It detects and decodes legacy computer-music formats: archives, compressors, snapshots, disk images, emulation dumps. Each recognized track is played back via chip-accurate emulation.

The pipeline is plugin-driven and format-agnostic. It targets CLI, Qt, and Android front-ends.

Actors:
- end user (listener with loop/seek/convert options)
- converter (exports interchange formats)
- data analyst (scans raw dumps for embedded modules)

## Pipeline

```
Raw data
   │
   ▼
[extraction] Scanner / [orchestration] Service.DetectModules
   │  decoders (formats) match at offsets
   ▼
[format-recognition] chiptune / archived / packed / image containers
   │
   ▼
[orchestration] PlayerPlugin / ArchivePlugin, DataLocation resolution
   │
   ▼
[player] Module::Holder  ──►  Renderer  ──►  device event streams
   │                                        │
   ▼                                        ▼
[device-emulation] chip emulators ──► Sound::Chunk (via mixers)
   │
   ▼
[sound-output] gain / fade / resample  ──►  Backend
   │
   ▼
[frontends] zxtune123 / zxtune-qt / zxtune-android / xtractor
```

## Glossary

- **Module**: Playable unit produced from recognized chiptune data. Exposed via `Module::Holder`. Carries `Information`, properties, and renderer factory. [`holder.h`](/src/module/holder.h)
- **Renderer**: Runtime playback engine for a module. Advances state frame-by-frame. Supports seek. [`renderer.h`](/src/module/renderer.h)
- **Plugin**: Stateless decoder provider. Has immutable `PluginId`, description, capabilities. Two families: `PlayerPlugin`, `ArchivePlugin`. [`plugin_attrs.h`](/src/core/plugin_attrs.h)
- **DataLocation**: Data plus resolved path and plugins chain. Enables nested resolution and error reporting. [`data_location.h`](/src/core/data_location.h)
- **Decoder**: Recognizes a format family. Declares a search `Format`. Decodes into a typed `Container`. [`chiptune.h`](/src/formats/chiptune.h)
- **Device**: Emulated sound chip with register-level interface. `Dumper` variants record register writes. [`aym.h`](/src/devices/aym.h)
- **Chunk**: Move-only block of `Sample`s (16-bit stereo). Produced by renderers, consumed by sound pipelines. [`chunk.h`](/src/sound/chunk.h)
- **Backend**: Audio output sink. Exposes playback/volume control and analyzer. [`backend.h`](/src/sound/backend.h)
- **Container chain / Subpath**: Nested containers (archive > compressor > snapshot > chiptune). Expressed as an `Analysis::Path` plus plugins chain. Used for addressing and attribute reporting. [`attributes.h`](/src/module/attributes.h)
- **Detection**: Scanning raw data for plugin matches. Yields `Analysis::Result` (matched size / lookahead offset). [`result.h`](/src/analysis/result.h)

## Bounded Contexts

| Context | Responsibility |
|---|---|
| [Format Recognition](contexts/format-recognition.md) | Pattern matching and decoding of chiptunes, archives, packed data, images, multitracks; typed containers with checksums |
| [Module (Player)](contexts/player.md) | Playable abstraction: holder, information, renderer, state; track model (order list → patterns → lines → cells) and stream/emulation playback |
| [Device Emulation](contexts/device-emulation.md) | Chip-accurate emulators and dumpers |
| [Sound Rendering & Output](contexts/sound-output.md) | Mixing, resampling, gain/fade/silence processing, and OS/device audio backends |
| [Core Orchestration](contexts/orchestration.md) | Plugin registry, capability taxonomy, `Service` open/detect flows, location and additional-files resolution |
| [Analysis / Extraction](contexts/extraction.md) | Scanning raw data with decoder fan-in; path algebra for addressing nested data |
| [Front-ends](contexts/frontends.md) | End-user apps: CLI, Qt, Android; converter and analysis tools |

The contexts are deliberately decoupled:
- format recognition knows nothing of playback
- the player knows nothing of OS audio
- device emulation is pure chip simulation
- orchestration wires them via plugins and capabilities

## Related documentation

- [`doc/features/channels-muting.md`](/doc/features/channels-muting.md) — per-channel muting support across formats.
