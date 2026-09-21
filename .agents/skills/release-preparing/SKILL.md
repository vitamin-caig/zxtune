---
name: release-preparing
description: Use for zxtune release numbering, changelog consistency, and store release notes. Trigger keywords: release, public, changelog, GooglePlay, fastlane, hotfix, release notes.
---

# zxtune release preparing

This skill formalizes the maintainer-run workflow for preparing a zxtune
release: deriving the next release number, keeping the in-repo changelog
consistent, and generating the Android release notes (GooglePlay cumulative
changelogs per language, including the German translation, and per-build
fastlane changelogs). It is advisory: it drafts numbers and file contents
as text for the user to add, and must NOT write files, edit market or
fastlane files, bump versions, create commits or tags, or push. All of that
happens only on explicit user request; the skill produces proposals and
validations.

## Ground facts

- Source of truth for a release's number is a git tag `r<N>` (for example
  `r5100`). Everything derives from `<N>`: in-repo changelog section `Rev<N>`,
  GooglePlay notes `Build <N>`, fastlane per-build changelog `<N>.txt`, market
  changelog headers. The tag message encodes the release type (see tags below).
- `apps/changelog.txt` and the GooglePlay changelog are written for users:
  readers use them to decide whether to install the update. Entries must
  convey what changes for the user and why it matters (new features, fixed
  blockers, impactful improvements), never internals.
- `apps/changelog.txt` is the in-repo, Debian-style changelog (newest-first).
  Section headers: `--- Rev<N> from DD.MM.YYYY`. Bullet lines start at the
  first column. Markers: `[+]` (new feature), `[*]` (fix/improvement),
  `[-]` (removed feature/rudiment). A prefix (`zxtune-android:`,
  `zxtune-qt:`, `zxtune123:`, `xtractor:`) marks entries scoped to one app;
  unprefixed entries are core/general. Entries order within a section by
  the composite key <app, change_type>: app groups in the order core,
  `xtractor:`, `zxtune-qt:`, `zxtune123:`, `zxtune-android:`; within a
  group, change_type in the order add (`[+]`) first, then fix (`[*]`),
  then del (`[-]`). Newly supported formats are
  listed by their identifiers (file extensions), for example "Supported
  FOO,BAR and BAZ formats playback".
- `apps/changelog.txt` lists user-visible changes only; changes hidden behind
  internals (library update without user effect, refactoring) do not belong.
  When in doubt about a change, inspect the commit and reason about its
  user-visible effect: an internal repack is not user-visible by itself,
  while the reduced binary size it brings is. Describe the user-visible
  problem and its effect, never the internal fix mechanism - e.g. "Fixed
  playback of damaged files", not "Fixed buffer handling in the decoder".
  A fix for a regression that
  was introduced and fixed within the same release is not user-visible
  either: the bug never shipped, so there is nothing to announce.
- Group similar changes into a single entry while writing the changelog
  section: bullets of the same kind that share a common frame (same verb
  phrase, e.g. "fixed playback of") merge into one bullet, the frame kept
  once and each bullet's distinguishing part enumerated in parallel, comma-
  separated with "and" before the final fragment where it reads naturally:

      * fixed playback of some FOO tracks
      * fixed BAR playback crash
      * fixed playback of corrupted BAZ files

  becomes:

      * fixed playback of some FOO, crashed BAR, corrupted BAZ tracks

  and:

      * fixed opening of DDD images
      * fixed opening of malformed EEE archives

  becomes:

      * fixed opening of DDD images and malformed EEE archives

  A merged bullet must keep the full enumeration it absorbs; an item that
  does not fit a group is listed on its own. Several rare crash fixes can
  also fold into one "fixed crashes" bullet; a single or blocker-severity
  crash may get its own entry, always phrased by its user-visible effect and
  without technical details (no internal names or subsystems). Newly
  written wording avoids internal names; pre-existing precedent wording
  from older sections is kept verbatim by the market transform. When crash
  severity is unclear, analyze the commit diff first, give the user an
  estimation (e.g. how likely/visible the crash is), and ask whether to keep
  the entry separate or fold it into the grouped crash fix. Grouping happens
  only while writing the changelog section; the market transform below never
  regroups, it copies mechanically.
- When attributing a change, do not confuse libvgm (VGM/VGZ playback
  emulator) with vgmstream (a separate streaming game-audio decoder): both
  are touched by commits but affect different formats. Inspect the commit
  before naming the affected subsystem - a vgmstream hang fix is not a VGM
  playback fix.
- Every tagged release `r<N>` is a public build; the number and tag message
  reflect the release size/type, not public-ness. Main releases produce
  changelog records (changelog section, GooglePlay block, fastlane file);
  hotfixes are tag-only, tagged without changelog records. Store publishing
  (GooglePlay, F-Droid/IzzyOnDroid) treats the per-version changelog as
  optional - a missing entry is simply not displayed - and only requires a
  monotonic `versionCode` and APKs attached to releases.
- The GooglePlay/F-Droid `Build <N>:` block derives from the `Rev<N>`
  section by a mechanical transform: drop whole bullets scoped to other
  apps (`xtractor:`, `zxtune-qt:`, `zxtune123:`), keep unprefixed (shared
  core) and `zxtune-android:` lines; remove the `zxtune-android:` prefix;
  turn typed bullets into plain asterisks and lowercase the leading
  keyword; reorder by change_type only (add < fix < del), stable with
  respect to the changelog order for ties. Nothing else changes: wording,
  enumerations, and precedent phrasing stay verbatim. The add/fix/remove
  meaning of the markers lives only in the changelog; the market bullet
  does not carry it.
- Look at the last few `apps/changelog.txt` sections and the latest git tags
  and their messages for the concrete current conventions before proposing
  anything.

## Numbering rules

Let `Last` be the release number of the last release (last tag `r<N>`). Main
releases are multiples of 10.

- Main (major) release: the next multiple of 10 higher than `Last` (for
  example after r5080/r5100 -> r5090/r5110). Label: `r<N>`. Tag message
  guidance: `Public build.`, `Public build. Major.`.
- Hotfix (minor) release: the next free integer after `Last` (for example
  r5000 -> r5001). These are NOT multiples of 10. Tag message examples:
  `Hotfix.`, `GooglePlay hotfix.`, `Hotfix for Android.`.
- The chosen number is used verbatim everywhere: tag `r<N>`, changelog
  section `Rev<N>`, GooglePlay `Build <N>`, fastlane `<N>.txt`,
  market headers.

## Changelog editing (apps/changelog.txt)

Follow the existing Debian-style format and newest-first ordering.

1. For a main release, add a section header `--- Rev<N> from DD.MM.YYYY` at
   the top; use the current date. Hotfixes do NOT get a section (tag only).
2. Add unindented bullet lines (`[+]`/`[*]`/`[-]` markers) to the new
   section, following the entry ordering and format conventions from the
   Ground facts.
3. List only user-visible changes, phrased by the user-visible problem and
   its effect (see Ground facts): read the commit and reason about how it
   affects the user; internals (emulator or library rework without user
   effect) and fixes of the release's own regressions stay out. Group
   similar changes per the Ground facts conventions (rare crashes collapse
   into one entry, a single blocker-severity crash may stand alone); when
   in doubt about crash severity, analyze the diff, give an estimation, and
   ask the user.

## Android release notes generating

The skill drafts the release notes records as text; the user adds them to the
store/upload flow.

### GooglePlay/F-Droid cumulative changelog (market)

- The `Build <N>:` block is the `Rev<N>` section transformed by the rule
  from the Ground facts: drop whole bullets scoped to other apps, strip the
  `zxtune-android:` prefix, turn typed bullets into plain asterisks with the
  leading keyword lowercased, and reorder by change_type only (adds first,
  then fixes, then removals). Wording, enumerations, and precedent phrasing
  are kept verbatim; grouping happens only at changelog creation, never in
  the block. Produced for main builds only; hotfixes get no block.
  Example (transform):

      [-] zxtune-qt: dropped Wizard X support              <- dropped
      [*] Fixed playback of some tracks
      [+] zxtune-android: supported new online catalogue

  becomes:

      * supported new online catalogue
      * fixed playback of some tracks

  (the Android addition leads over the core fix: change_type outweighs the
  app after removal)

- The skill drafts the `en` block from the changelog section and proposes the
  `de` and `ru` translations of the same content (hand-written, not
  mechanical); the user reviews and adjusts them. The three languages must
  stay in sync.
  Header per language: `Build <N>:` (en) / `Билд <N>:` (ru) / `Veröffentlichung <N>:`

### fastlane per-build changelog (apps/zxtune-android/fastlane)

- Per-build files:
  `apps/zxtune-android/fastlane/metadata/android/{en-US,ru-RU}/changelogs/<N>.txt`
  (one file per main build; hotfixes get no file). fastlane supports en-US and
  ru-RU only; German is NOT added to fastlane.
- Content is a 1:1 copy of that build's market changelog block for the
  corresponding language (en-US <-> en, ru-RU <-> ru), including the
  `Build <N>:` header line. The skill drafts the file content; the user adds
  the file.

## Workflow (advisory; propose first)

1. Determine the last release tag: `git tag --sort=-version:refname | head`.
   Collect changes since it: `git log <last-tag>..HEAD --oneline`. The last
   tag may be a hotfix, and the tagged commit is already released: it and
   everything reachable from it must never be listed or mentioned for the
   upcoming release. Always use the last tag overall as the range start -
   never the previous main tag - and never refer to changes outside the
   range.
2. Estimate the release size: major (user-visible new features, catalogue
   additions) or hotfix (targeted fixes). Both are public builds. Choose the
   number: main (next multiple of 10) or hotfix (next free integer).
3. Report the commits behind each proposed changelog entry: map every draft
   bullet to the commit(s) it stands for, separately for the in-repo
   changelog and for the Android-relevant GooglePlay block. For every `[*]`
   bullet, verify the bug already existed at the last release tag: the buggy
   line must be present in `r<Last>` and not be introduced by an earlier
   commit in the range; check with e.g. `git show r<Last>:<file>` or
   `git log -S <symbol>`. Regressions introduced and fixed within the same
   range are dropped; only pre-existing bugs are user-visible. Follow the
   Ground facts ambiguity path (ask the user) when the history is unclear.
4. Propose the release number `<N>` and the tag message. For a hotfix the
   proposal ends here: no changelog section, GooglePlay block, or fastlane
   files are drafted (see Changelog editing step 1).
5. Draft the `apps/changelog.txt` `Rev<N>` section (if main) and the
   per-language Android release notes (GooglePlay cumulative en/de/ru +
   fastlane per-build changelogs for en-US/ru-RU), proposing the German
   translation for review.
6. Present the full proposal for review; do NOT write files, bump versions,
   tag, or commit without explicit confirmation.

## Verification

- In-repo changelog newest section is `Rev<N>` with the current date.
- Every item of the `Rev<N>` section (shared core and `zxtune-android:`)
  appears in the `Build <N>:` block exactly once with the same wording and
  format enumerations; other-app bullets are absent.
- GooglePlay market changelog `Build <N>:` block text equals the fastlane
  `changelogs/<N>.txt` for the same build (en-US vs en, ru-RU vs ru).
- The en/de/ru market blocks list the same items.
- A hotfix produces no new records: no changelog section, no `Build <N>:`
  block, no fastlane files for its number.
