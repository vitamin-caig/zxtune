# Analysis / Extraction Bounded Context

Scanning raw data for embedded modules and path algebra for addressing nested data. Sources: [`src/analysis`](/src/analysis).

## Aggregates & Entities

### Scanner

- **Identity / Attributes**: Collects decoders (`AddDecoder` for archived/packed/image/chiptune). Scans a source container via `Scan(source, target)`, reporting matches to a `Target` ([`scanner.h`](/src/analysis/scanner.h)).
- **Invariants**: `Target::Apply` is invoked per match with the matching decoder, byte offset, and decoded container. Unmatched bytes fall back to `Apply(offset, data)`.
- **Relationships**: driven by the orchestration context and by front-ends (xtractor). Drives format-recognition decoders.

### Analysis::Result

- **Identity / Attributes**: `GetMatchedDataSize()` (size of input data consumed at current position). `GetLookaheadOffset()` (offset for the next check) ([`result.h`](/src/analysis/result.h)).
- **Invariants**:
  - `GetMatchedDataSize()` returns 0 if data does not match at the beginning.
  - `GetLookaheadOffset()` returns > 0 when the current position does not match (up to input size if no match at all). Returns 0 on a match ([`result.h`](/src/analysis/result.h#L32)).
- **Relationships**: produced by `Plugin::Detect` (orchestration) and by the scanner. Consumed to advance detection loops.

### Analysis::Path

- **Identity / Attributes**: Ordered list of path elements (`Elements()`, `AsString()`). Navigation via `Append(element)`, `Extract(startPath)`, `GetParent()`. Parsed with `ParsePath(str, separator)` ([`path.h`](/src/analysis/path.h)).
- **Invariants**:
  - `Append` returns a new, always non-null path. Never mutates the current one ([`path.h`](/src/analysis/path.h#L40)).
  - `Extract` returns non-null only when the current path starts with the given prefix (full match included).
  - `GetParent` returns `Ptr()` when empty, otherwise non-null (possibly empty) parent.
- **Relationships**: value object used by `DataLocation` (orchestration), subpath resolution, and additional-files resolution.

### Analysis::Node (xtractor model)

- **Identity / Attributes**: `Name()`, `Data()` (never empty), `Parent()`. Root/subnode factories `CreateRootNode`, `CreateSubnode` ([`main.cpp`](/apps/xtractor/main.cpp#L59)).
- **Invariants**: Non-root nodes always have a parent. Data is always present.
- **Relationships**: front-end tree model over scanned data in the xtractor app. Each node maps to a matched offset/decoder.

## Value objects

- **`Analysis::Path`** — immutable, structurally shared path (see above).
- **`Binary::ScanningFormat`** — format pattern supporting `NextMatchOffset` for forward search ([`format.h`](/src/binary/format.h#L32)).

## Related contexts

- [Format Recognition](format-recognition.md) — scanners fan in `Formats::*::Decoder` instances and consume their containers.
- [Orchestration](orchestration.md) — `Service::DetectModules`/`OpenModule` reuse `Analysis::Result` and `Analysis::Path` for detection and subpath resolution.
- [Front-ends](frontends.md) — xtractor builds `Analysis::Node` trees from scanner results. CLI uses path-based item addressing.

## Open questions

- `Analysis::Result` "temporary solution with negative lookahead offsets for statistic collecting" (TODO in [`result.h`](/src/analysis/result.h#L19)). Semantics of negative lookahead are unspecified.
- No documented upper bound on scanner work or total matched data volume (pathological inputs).
