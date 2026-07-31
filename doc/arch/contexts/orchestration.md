# Core Orchestration Bounded Context

Wires decoders into plugins, resolves data locations, and exposes open/detect services. Sources: [`src/core`](/src/core).

## Aggregates & Entities

### Service

- **Identity / Attributes**: Front-facing facade: `OpenData(data, subpath)`, `OpenModule(data, subpath, initialProperties)`, `DetectModules(data, callback)`, `OpenModule(data, subpath, callback)` ([`service.h`](/src/core/service.h)).
- **Invariants**:
  - `OpenModule` throws if no module is found ([`service.h`](/src/core/service.h#L32)). `OpenData` throws if data cannot be resolved ([`service.h`](/src/core/service.h#L29)).
  - Subpath resolution walks archive plugins until the requested `Analysis::Path` is fully consumed. Failure to resolve any component throws ([`service.cpp`](/src/core/src/service.cpp#L239)).
  - Detection is first-match-wins over plugin enumeration order ([`service.cpp`](/src/core/src/service.cpp#L327)).
- **Relationships**: created from a `Parameters::Accessor`. Drives `PlayerPlugin`/`ArchivePlugin` registries. Resolves additional files for multifile modules via `ResolveAdditionalFilesAdapter` ([`service.cpp`](/src/core/src/service.cpp#L41)).

### DataLocation

- **Identity / Attributes**: `GetData()`, `GetPath()` (subpath within top-level dump), `GetPluginsChain()` (provenance of container plugins) ([`data_location.h`](/src/core/data_location.h)).
- **Invariants**: Every data piece has a definite location. Nested locations are created per container plugin applied ([`location_nested.cpp`](/src/core/src/location_nested.cpp)).
- **Relationships**: produced by `CreateLocation(data)` / `CreateNestedLocation(parent, ...)`. Consumed by plugin `Detect`/`TryOpen` and by the extraction context's `Scanner`.

### Plugin (base) / PlayerPlugin / ArchivePlugin

- **Identity / Attributes**: `Id()` (`PluginId`), `Description()`, `Capabilities()`. Player plugins add `GetFormat()`, `Detect(params, location, callback)`, `TryOpen(params, data, initialProperties)`. Archive plugins add `TryOpen(params, location, pathToOpen)` and process nested data via `ArchiveCallback` ([`plugins/player_plugin.h`](/src/core/plugins/player_plugin.h), [`plugins/archive_plugin.h`](/src/core/plugins/archive_plugin.h)).
- **Invariants**: Capabilities are a bitmask taxonomy: category (`MODULE`/`CONTAINER`), module type (`TRACK`/`STREAM`/`MEMORYDUMP`/`MULTI`), device family, conversion target, and traits (MULTIFILE / DIRECTORIES / PLAIN) ([`plugin_attrs.h`](/src/core/plugin_attrs.h)).
- **Relationships**: `PlayerPlugin::Enumerate()` / `ArchivePlugin::Enumerate()` provide registries. Wrappers around format-recognition decoders. Produce `Module::Holder`s (player) or nested `DataLocation`s (archive).

### DetectCallback / ArchiveCallback

- **Identity / Attributes**: `CreateInitialProperties(subpath)`, `ProcessModule(location, plugin, holder)`, `ProcessUnknownData(location)`, `GetProgress()` ([`module_detect.h`](/src/core/module_detect.h)). Archive callbacks add `ProcessData(location)`.
- **Relationships**: provided by callers (front-ends) and adapted internally (e.g. `OpenModuleCallback`, `ResolveAdditionalFilesAdapter`, `RecursiveDetectionAdapter`).

### AdditionalFiles resolution

- **Identity / Attributes**: `AdditionalFilesSource::Get(name)` resolves a sibling file by (sub)path. Results are cached per name ([`service.cpp`](/src/core/src/service.cpp#L95)).
- **Invariants**: For a multifile module, resolution is attempted against the parent directory of the module's location. Failure degrades to `ProcessUnknownData` ([`service.cpp`](/src/core/src/service.cpp#L66)).
- **Relationships**: links a `Module::AdditionalFiles` capability (player context) to archive contents (format-recognition context).

## Value objects

- **`PluginId`** — strong string-typed plugin identifier, built via `""_id` literal ([`plugin_attrs.h`](/src/core/plugin_attrs.h#L18)).
- **Capabilities bitmasks** — category/type/device/conversion/traits constants with disjoint masks enforced by `static_assert`s.

## Related contexts

- [Format Recognition](format-recognition.md) — decoders are wrapped as plugins. Containers flow through nested locations.
- [Player](player.md) — orchestration produces `Module::Holder`s and resolves additional files.
- [Extraction](extraction.md) — uses `Analysis::Path`/`Result`. `DetectModules` recurses through nested locations.
- [Sound Output](sound-output.md) — `Sound::Service` creation is parameterized. Orchestration supplies global parameters.
- [Front-ends](frontends.md) — front-ends call `Service` directly.

## Open questions

- First-match-wins detection: no defined priority/ordering guarantee across registries for genuinely ambiguous data.
- Detection recursion has no stated depth or total-expansion bound for hostile inputs.
- Additional-files failure silently drops the module to unknown-data. Whether it surfaces to the UI is caller-dependent.
- `ATTR_CRC`/`ATTR_FIXEDCRC` identity semantics (raw CRC vs structural fingerprint) are not documented as interchangeable or version-stable.
