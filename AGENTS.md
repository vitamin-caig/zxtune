# Domains

* chiptune
* DSP
* hardware emulation

# Toolset

* git, GNU make
* C++20, STL

# Sources

## End-user apps and tools

- [`zxtune123`](apps/zxtune123) - CLI
- [`zxtune-qt`](apps/zxtune-qt) - GUI based on Qt 5
- [`zxtune-android`](apps/zxtune-android) - Android app

## Foundational

`include/` - headers with foundational types, no domain-specific declarations

## Core libraries

Subdirectories at `src/`. Each library consists of Makefile and public API headers.

All circular dependencies resolved via header-only interface layer. Libraries in a cycle must be modified together in a single change.

## Thirdparty components

Located at `3rdparty/`. Included files are ALWAYS referenced with prefix (e.g. `#include "3rdparty/vgm/emu/SoundEmu.h"`)

* DON'T read or change files unless explicitly asked
* DO preserve the existing file's style when modifying files
* DO match the style of adjacent files in the same subdirectory when adding new files

## Build system and related tools

Scripts for GNU make located at `make/`.

### Jumbo builds

Sources combining, allowed per-target via `jumbo.name`. ALL and ONLY sources with includes from `3rdparty/` should be listed at `jumbo.excludes`.
All sources should have different filenames upon the library.

## Non-goals

- `samples/` - sample data
- `regression/` - regression data
- `l10n/` - localization files

## Forbidden intermediate directories

- `bin/`
- `lib/`
- `pkg/`
- `obj/`
- `Builds/`

* DON'T access unless explicitly asked.
* DON'T list or read files at the project root.
* Note: the Android app build writes here at runtime and this is expected - Gradle `buildDir` is redirected to `pkg/<module>` (`make/android/project.gradle`) and release artifacts are published to `Builds/<version>/android`. These are generated outputs; don't hand-edit them, and don't treat their appearance as an unsolicited change.

## C++ style

C++ source files extensions are `.cpp` and `.h`.

DO use full paths starting from repository root, `include/` or `src/` directories
DON'T use relative or local includes
DON'T use redundand namespace qualifiers unless required for disambiguation
DON'T use anonymous namespaces

Run `make/tools/clang-format -i <path_to_file>` from the repository root for C++ style enforcement.
Exit code 0 means success (file reformatted or already clean). Non-zero means the file could not be parsed, fix syntax and retry.
If you cannot execute, follow the project's clang-format:
- 4-space indent
- trailing braces
- MaxColumnWidth=100
- PointerAlignment=Left

and apply rules manually.

## Commentaries

Commentaries should explain WHY code exists rather than what it does.

Good examples:

```c++
// Some of the modules (e.g. Story Map.psc) references more ornaments than really stored
const std::size_t maxOrnaments = (ornamentsTableEnd - ornamentsTableStart) / sizeof(uint16_t);
```

```c++
case 3:
  // copyright/publisher really
  props.SetComment(Strings::SanitizeMultiline(tuneInfo.infoString(2)));
```

```c++
// If looped, do not allow fadein
// to avoid absolute silence
const auto part1 = doneLoops ? Preamp : Fading.GetFadein(Preamp, posAfter);
```

```c++
// do not add/add to 'success' error
if (e && *this)
```

```c++
// selection->currentIndex() does not work
// items are orderen by selection, not by position
QModelIndexList items = selection->selectedRows();
```

Bad examples:

```c++
// add text to set
auto text = templ.substr(textBegin, fieldBegin - textBegin);
FixedStrings.emplace_back(text);
```

```c++
// check if required to revert table
const bool doRevert = !id.empty() && *id.begin() == REVERT_TABLE_MARK;
```

## Refactoring

DON'T skip commentaries while moving blocks of code between or within files.

## Agent process

- Read every applicable AGENTS.md: the repo-root one and any nested one in the directory you work in. Injected root instructions don't substitute for verification.
- The skill tool injects a summary; after loading a skill, read the full `SKILL.md` on disk under `.agents/skills/<name>/` before acting.
- `glob` doesn't descend into dot-directories (`.agents/`, `.git/`) - confirm their contents with `ls -laR`.

# Do / Don't for Agents

- DON'T change git repo (`commit`, `reset`, `cherry-pick` commands disallowed) unless given permission for single action
- DON'T change generated files (`.inc`, `*_api.h`, `*_api_dynamic.cpp`, `.qm`, `.mo`) in accessible directories
