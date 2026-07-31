# Format Recognition Bounded Context

Recognition and decoding of raw binary data into typed containers. Sources: [`src/formats`](/src/formats), [`src/binary`](/src/binary).

## Aggregates & Entities

### Decoder (per format family: Chiptune, Archived, Packed, Multitrack, Image)

- **Identity / Attributes**: `GetDescription()`, `GetFormat()` (search pattern, never empty), `Check()` (fast consistency check), `Decode(rawData)`.
- **Invariants**:
  - `GetFormat()` must never be empty ([`chiptune.h`](/src/formats/chiptune.h#L47)).
  - `Check()` returns `false` only when data is definitely wrong. A non-false answer is not a guarantee.
  - Decode result is always a **subcontainer** of the input data ([`chiptune.h`](/src/formats/chiptune.h#L57), [`archived.h`](/src/formats/archived.h#L89)).
  - `Packed::Decoder::Decode` consumes exactly `Container::PackedSize` first bytes of input ([`packed.h`](/src/formats/packed.h#L51)).
- **Relationships**: registered by the orchestration context as plugins. Driven by the extraction context's `Scanner`. Containers are consumed by the player context.

### Chiptune::Container

- **Identity / Attributes**: `Checksum()` (raw data CRC32) and `FixedChecksum()` (structural fingerprint of decoded internals).
- **Invariants**: Always a subcontainer of the decoded input.
- **Relationships**: subtype of `Binary::Container`; base type for `Multitrack::Container` ([`multitrack.h`](/src/formats/multitrack.h#L22)).

### Archived::Container / Archived::File

- **Identity / Attributes**: Archive identified by its raw bytes. Files keyed by `GetName()` (may contain path separators), with `GetSize()` and `GetData()`.
- **Invariants**: File data is non-empty when extracted ([`archived.h`](/src/formats/archived.h#L38)). Walking order is kept stable ([`archived.h`](/src/formats/archived.h#L60)).
- **Relationships**: `Container` is the parent aggregate. `File` is its child. A `Walker` visits files via `ExploreFiles`; a single file is fetched by `FindFile`.

### Multitrack::Container

- **Identity / Attributes**: `TracksCount()` and `StartTrackIndex()` (0-based index of first track).
- **Relationships**: extends `Chiptune::Container` ([`multitrack.h`](/src/formats/multitrack.h#L22)). Represents formats with non-divideable tracks.

### Packed::Container

- **Identity / Attributes**: `PackedSize()` (size of source data it was unpacked from).
- **Invariants**: `PackedSize()` is always > 0 ([`packed.h`](/src/formats/packed.h#L30)).
- **Relationships**: extends `Binary::Container`; produced by `Packed::Decoder`.

### Binary::Format / ScanningFormat

- **Identity / Attributes**: Pattern description. `Match(data)` checks conformance.
- **Invariants**: `NextMatchOffset(data)` returns the matched offset or data size, always > 0 ([`format.h`](/src/binary/format.h#L39)).
- **Relationships**: value object used by every `Decoder` to describe its format. Enables scanning in the extraction context.

### Binary::Container / View

- **Identity / Attributes**: Read-only byte range. View shares memory of its parent.
- **Invariants**: Subcontainer lifetime depends on the parent buffer. Ownership contract is not documented at interface level.
- **Relationships**: base type of all decoded containers. Carries `Checksum`/CRC helpers in the binary library.

## Related contexts

- [Orchestration](orchestration.md) — wraps decoders as `PlayerPlugin`/`ArchivePlugin` and registers them.
- [Extraction](extraction.md) — drives decoders via the `Scanner` across raw data offsets.
- [Player](player.md) — consumes decoded chiptune containers to build `Module::Holder`s. Conversion dumpers feed back into archived/packed-style formats.

## Open questions

- Plugin/decoder ordering and ambiguity policy: the first matching decoder wins (see orchestration). No defined resolution for genuine multi-format overlap.
- `Checksum()` vs `FixedChecksum()` semantics are not documented as stable across tool versions. `FixedChecksum` is likely version-sensitive and unsafe as durable identity.
- `Check()` vs `Decode()` contract — consumers that may skip/trust `Check` are not enumerated.
- Subcontainer ownership: who must keep the parent buffer alive is unspecified.
