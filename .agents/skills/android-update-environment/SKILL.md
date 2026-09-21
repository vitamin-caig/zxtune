---
name: android-update-environment
description: Use when updating the Android build environment for apps/zxtune-android — AGP, Gradle, JDK, Kotlin/KSP, compileSdk/targetSdk, or AndroidX/dependency versions, either individually or together. Trigger keywords: targetSdk, compileSdk, AGP, Gradle, Kotlin, KSP, minSdk, dependency update, AndroidX, buildToolsVersion, library bump, upgrade dependencies, environment update, toolchain bump.
---

# Updating the Android build environment

## Ground facts

- SDK levels live ONLY in `apps/zxtune-android/zxtune/build.gradle`:
  `compileSdk`, `targetSdkVersion`, `buildToolsVersion`. Shared scripts under
  `make/android/` do not set them.
- AGP is pinned via the `com.android.tools.build:gradle` classpath entry in
  `apps/zxtune-android/build.gradle`.
- Gradle wrapper version: `apps/zxtune-android/gradle/wrapper/gradle-wrapper.properties`
  (`distributionUrl`) — it is what actually runs the build.
- Kotlin is `kotlin_version` in the `ext` block of
  `apps/zxtune-android/build.gradle`; KSP is declared as a pair tied to it
  (`"${kotlin_version}-<ksp-version>"`) and must move together with it.
- Dependency versions: `apps/zxtune-android/build.gradle` (`ext` block) and
  the module's `dependencies` block. Pinned libs carry
  `//noinspection GradleDependency minsdk=<N>` comments documenting the floor
  that forced the pin — preserve them, update the number if the analysis
  changes.
- The app builds several min-SDK flavors (configured via `minSdks` in
  `make/android/nativelibs.gradle`). Fetch the actual values and the
  flavor -> artifact-type mapping from the build scripts; do not hardcode
  them. The lowest configured minSdk is a hard constraint: any dependency
  bump must keep the app runnable there. Raising it is a product decision
  made only when required.
- `make/docker/builds/Dockerfile.android` pins the containerized toolchain
  and MUST be kept in sync: `platform`/`build_tools` mirror
  `compileSdk`/`buildToolsVersion`, its `ndk` env pins an OLDER NDK ("last
  supporting minSdk=16") and generates the container's
  `local.properties`/`gradle.properties` (see "Docker build sync").
- No `jvmTarget`/`javaVersion`/`compileOptions` are pinned — the bytecode
  target follows the toolchain/JDK defaults; a Kotlin/AGP/JDK change can move
  it silently.

## Startup: current state + pages to fetch

Before any decision, read the current values from the build files (never from
memory) and fill the "current" column of the environment version table:

- `apps/zxtune-android/zxtune/build.gradle`:
  `compileSdk`, `targetSdkVersion`, `buildToolsVersion`
- `apps/zxtune-android/build.gradle`: AGP classpath, `kotlin_version` (ext),
  KSP pair, dependency versions and their pin comments
- `apps/zxtune-android/gradle/wrapper/gradle-wrapper.properties`:
  `distributionUrl`
- `make/android/nativelibs.gradle`: `minSdks` values per flavor
- `make/docker/builds/Dockerfile.android`: `platform`, `build_tools`, `ndk`
  env pins
- `apps/zxtune-android/local.properties`: the local NDK version

Fetch the reference pages (via `fetch_url.sh`) as the stages need them:

| stage | page |
|-------|------|
| driver | `https://developer.android.com/google/play/requirements/target-sdk` (Play deadline/enforcement) |
| behavior audit | `https://developer.android.com/about/versions/<N>/behavior-changes-<N>` and `.../behavior-changes-all` (`<N>` = the targetSdk read above) |
| AGP | `https://developer.android.com/build/releases/gradle-plugin` (release notes; AGP↔Gradle/JDK + max compileSdk) |
| Gradle | `https://docs.gradle.org/<wrapper>/release-notes.html` (`<wrapper>` = the `distributionUrl` version) |
| Kotlin | `https://kotlinlang.org/docs/releases.html` |
| KSP | `https://github.com/google/ksp/releases` |
| libraries | the `maven-metadata.xml` templates in "Libraries and minSdk" |

## Update modes

Choose one mode per run; it sets how far every stage moves:

| mode | targetSdk/compileSdk | toolchain (AGP/Gradle/JDK, Kotlin/KSP) | libraries (AndroidX & co) |
|------|----------------------|----------------------------------------|---------------------------|
| conservative | only what is forced — check the Google Play targetSdk compliance deadline (page in "Startup: current state + pages to fetch") | lowest versions that satisfy the requirement | minimal patch/minor; no majors |
| realistic | migrate to the latest targetSdk on purpose | latest stable, mutually compatible | latest stable incl. bugfixes; avoid majors unless needed |
| optimistic | latest targetSdk, optionally raising the minSdk floor to unlock majors | newest stable across major lines | newest affordable; majors allowed if compatible |

Default is conservative. "Keep the app current" → realistic; "take whatever
the environment allows" → optimistic.

## Decide the mode first

Before changing anything, estimate each mode — concrete changes (versions,
SDK levels, audits) and risks (breaks, new floors, device loss, verification
cost) — present the three-mode comparison and ASK which to apply (question
tool). Feed Google Play compliance pressure and build time into the
recommendation, but do not decide for the user.

## Retrieving Google developer pages

- Always use `.agents/skills/fetch_url.sh` (never bare `curl`/`webfetch`);
  it uses Firecrawl and returns markdown — parse the result directly:
  `.agents/skills/fetch_url.sh "<url>"`.
- On failure, retry once; if it still fails, mark the item **unresolved** in
  the report — never substitute a guess or a `websearch`.
- If the Play requirements page is unreachable, the deadline is unresolved:
  pause — the deadline drives the "driver first" stage.

## Decision flow (mode-aware)

Walk the stages in order; each caps the ones after it. How far each moves is
set by the mode; by default (conservative) nothing moves unless forced, and
then as little as possible.

1. **The driver first**: what pushes the update — for conservative an
   enforced targetSdk, a critical bugfix, or a planned minSdk floor bump (a
   product decision via `minSdks` in `make/android/nativelibs.gradle`);
   realistic/optimistic just pick the targetSdk per the mode table.
2. **Toolchain**: AGP + Gradle + JDK + Kotlin/KSP pair per the mode, as the
   lowest consistent set unless the mode says otherwise
   (see "AGP / Gradle / JDK", "Kotlin / KSP").
3. **SDK levels**: apply compileSdk/targetSdk and run the behavioral audit
   across every level in between (see "SDK levels and behavioral changes").
4. **Dependencies**: per the mode, always respecting the min-SDK floor
   (see "Libraries and minSdk").

At each step record the constraints handed downward (e.g. "chosen AGP needs
compileSdk X", "chosen Kotlin needs AGP Y") so later steps reuse them instead
of re-researching.

---

## AGP / Gradle / JDK

### Resolve version compatibility

- Pick per the mode table: conservative = LOWEST AGP+Gradle+JDK triple that
  satisfies the requirement; realistic = latest stable on the current major
  line; optimistic = newest stable across majors.
- Build the triple from the official AGP-to-Gradle table + required JDK.
- Compare against the configured compileSdk: AGP documents (and warns about)
  the newest compileSdk/`buildToolsVersion` it supports; a compileSdk bump
  usually travels with an AGP bump.

### Assess breaking changes

- Major AGP versions remove/rename DSL blocks and change defaults (namespace
  handling, `buildConfig`, R-class generation, manifest-merge rules) — review
  the release notes + DSL reference between current and candidate, not just
  latest.
- Check toolchain bits the release pulls in: NDK/CMake version requirements,
  `buildToolsVersion`, Kotlin/KSP minimums.
- Treat a major line as a point to avoid unless no older line supports the
  required compileSdk/JDK set; otherwise stay on the current line's highest
  patch.
- Re-check release notes for anything touching shrinking/proguard and
  resource processing — AGP majors sometimes restrict such schemes.
- `android.newDsl`, `android.disallowKotlinSourceSets`,
  `android.onlyEnableUnitTestForTheTestedBuildType` are AGP-sensitive flags
  in BOTH `apps/zxtune-android/gradle.properties` and the Dockerfile's
  generated `gradle.properties` — verify they still exist/mean the same on
  an AGP bump.

### Hard floors to record

AGP/Gradle/JDK compatibility is a hard floor — record the triple + bounds in
the environment version table; a later stage needing more bumps it upward
(see "Conflict resolution"). The wrapper is the real entry point; a classpath
version needs the wrapper version it requires.

---

## Kotlin / KSP

### Pick the candidate version

- Pick per the mode table: conservative = LOWEST Kotlin/KSP pair supported by
  the AGP/Gradle in use and the required compileSdk; realistic = latest
  stable; optimistic = newest stable coordinated with newest compatible
  AGP/Gradle.
- Confirm the wrapper/AGP support the candidate; if they need a bump first,
  do it in stage 2 above.
- Check AndroidX/coroutines compatibility for the candidate pair so the
  dependency side is queued for stage 4.

### Assess language and compatibility changes

- Review the language changelog: new language/api default levels,
  deprecations promoted to warnings/errors, newly experimental/stabilized
  features, `@OptIn` changes.
- Look for compiler/runtime behavior changes (integer/string ops, JVM
  interop, reflection, default methods) and stdlib additions colliding with
  existing declarations.
- Nothing pins the language/api level — an upgrade silently flips to the new
  default; decide whether to pin `languageVersion`/`apiVersion` or accept it.

### Hard floors to record

Kotlin↔AGP/Gradle and KSP↔Kotlin pairings are hard floors — record them in
the environment version table. Kotlin and its KSP always move together (a
mismatch fails late, at the KSP task). In conservative mode stay at the
lowest pair; avoid language/api level changes that force source changes.

---

## SDK levels and behavioral changes

### Research behavioral changes for the new SDK version

Fetch both official pages for the target SDK `<N>` (apps targeting `<N>`;
all apps) — the URLs are listed once in "Startup: current state + pages to
fetch". If the `-all` page 404s, retry once, then mark it **unresolved** in
the report instead of substituting anything.

Classify each change as MUST address / verify only / no impact and map it to
code locations. Audit the recurring categories below — each bump adds,
splits, or tightens some, so record which actually changed:

- Runtime permissions: a release may add a permission or split an existing
  one (notification, media files, nearby devices, location granularity,
  background location, body sensors, health, local network). Check the
  manifest `uses-permission` entries and that each sensitive area the app
  touches is declared, requested, and handles denial.
- Storage and file access: scoped-storage rules, MediaStore queries/writes
  and legacy `DATA`-column usage, raw absolute-path access,
  `getExternalStoragePublicDirectory`, `file://` intents vs FileProvider.
- Background execution, services, and scheduling: service-start limits,
  `startForegroundService`/`startForeground` pairing with service type and
  matching `FOREGROUND_SERVICE_*` permission, manifest vs dynamically
  registered broadcast receivers, notification/intent trampolines, exact
  alarms and job/standby scheduling APIs.
- Component exposure and intents: explicit exported flag on every manifest
  component, `PendingIntent` mutability, implicit/untyped-intent
  restrictions, export flags on dynamically registered receivers, package
  visibility for external component queries.
- Network and connectivity: cleartext HTTP and the network security config,
  minimum TLS levels, local-network and device-discovery features
  (mDNS/NSD, Bluetooth, Wi-Fi), restrictions on hardware identifier access.
- UI and system bars: edge-to-edge enforcement, window insets, system-bar
  colors and translucent themes, Back button and predictive-back APIs,
  splash screens, notification channels/appearance, launcher icon formats,
  orientation and resizability.
- Runtime and platform APIs: signature changes on platform methods,
  OpenJDK refresh collisions (new methods/overloads that fail on older
  devices after recompile), non-SDK/reflection API use, libraries removed
  or replaced on the platform, WebView/default-component changes.

For each finding record the fix venue: manifest/resources, code, dependency
updates, or QA only.

Files that usually matter here: `AndroidManifest.xml`, `res/values*/styles.xml`,
`MainActivity.kt`, UI fragments, the media/device service classes, and the
Robolectric tests.

### Docker build sync (`make/docker/builds/Dockerfile.android`)

- Mirror `compileSdk`/`buildToolsVersion` changes from `zxtune/build.gradle`
  into the Dockerfile's `platform`/`build_tools` env vars and rebuild the
  image to verify the SDK components install.
- The Dockerfile pins its own OLDER NDK (`ndk=`, "last supporting
  minSdk=16") to serve the lowest min-SDK flavor — the repo's local
  `local.properties` may use a newer one; don't assume they match. Raise it
  only when the floor rises; record the floor in the version table.
- The container generates `local.properties`/`gradle.properties` from these
  pins; keep them consistent with the repo's own files.

Distinguish changes tied to the targetSdk value from those applying to ALL
apps; spell out whether each behavior activates only after the bump.

---

## Libraries and minSdk

### Find the latest available versions

- Google-hosted (AndroidX, AGP, flexbox): fetch
  `https://dl.google.com/dl/android/maven2/<group>/<artifact>/maven-metadata.xml`
- Everything else (jsoup, coroutines, robolectric, mockito, kotlin-gradle-plugin,
  KSP): `https://repo1.maven.org/maven2/<group>/<artifact>/maven-metadata.xml`

Filter stable releases (drop alpha/beta/rc/dev). Selection follows the mode:
conservative picks the LOWEST satisfying version (newest is informational);
realistic/optimistic take latest stable / newest affordable.

### Check the min-SDK requirement the candidate actually ships

- AARs embed it: download the candidate `.aar`, `unzip`, then read
  `AndroidManifest.xml` via
  `strings ... | grep -oP '(?<=minSdkVersion=")[0-9]+'`.
- Plain JARs (annotation-style libs, coroutines, html parsers, JVM test
  utils) have no embedded minSdk — consult release notes (Java/API floors are
  usually documented), then verify via build + unit tests on the lowest
  minSdk flavor variant.
- A safe AAR may drag in a transitive whose minSdk exceeds the floor —
  confirm via lint + the test task after the switch.
- Test-only deps are a special case: local JVM (`testImplementation`) and
  on-device (`androidTestImplementation`) deps never install below the
  flavors used for testing, so a higher minSdk there is tolerated; call this
  nuance out when reporting.

### Rules

- All modes: bump only if the configured minimum SDK keeps working
  (realistic/optimistic also need matching toolchain/SDK levels).
- Within the mode policy, prefer the smallest bump that satisfies it.
- Otherwise keep the pin + suppression comment, noting which version lifted
  the floor (so the threshold is obvious next bump).
- Staying put is a valid outcome — state it explicitly.
- The floor wins over the mode ambition on a clash; record it and re-check
  after the toolchain/SDK decisions (see "Conflict resolution").
- Transitive upgrades via a safe direct bump still count as changes.
- Avoid majors (new floors, behavior changes); bump only when forced.
- The floor can differ between flavors — check every flavor used for the
  artifact type.

---

## Verification

Prefer fast, native-free tasks:

- Unit tests (Robolectric): the base-minSdk-flavor task
  `./gradlew zxtune:test<PackagingFlavor><MinSdkFlavor>DevelopUnitTest`
  (scoped with `--tests <FqTestClass>`; resolve names via `./gradlew tasks`).
- Lint: `./gradlew lint<PackagingFlavor><MinSdkFlavor>Develop`.
- Config sanity: `./gradlew help` / `zxtune:tasks` / `zxtune:properties`.
- KSP failures surface at execution — run a KSP-consuming task (unit tests
  compile with KSP).
- If JDK/wrapper changed, check the bytecode target did not move quietly
  (`javap -v` on a compiled class).
- A wrapper/AGP major change may need one full `assemble*` run (catches
  variant/manifest-merging, packaging) — it triggers the repo-wide C++
  `make platform=android` build for every ABI.

---

## Conflict resolution

Each stage records its decision + the hard floors it relied on into the
environment version table; conflicts are resolved here, once:

- Hard floors beat mode ambitions in every mode: the min-SDK floor (from the
  build scripts), AGP↔Gradle/JDK, AGP↔compileSdk, Kotlin↔AGP/Gradle,
  KSP↔Kotlin, and each library's own minSdk.
- On a later-stage clash with an earlier choice, the later requirement wins
  and the earlier stage is re-evaluated on the tightened constraint set
  (same mode policy), repeated until stable — it converges because modes only
  relax ambitions, never floors.
- "Latest" is chosen only among versions satisfying every recorded floor; no
  floor is relaxed to fit a newer version.
- If no candidate satisfies the union of floors, it is a product decision
  (raise the floor, tolerate an older AGP, split flavors) — escalate, don't
  pick silently.

## Coordination rules

- Pins forced to move together change in ONE change set — never a partial,
  inconsistent set (Kotlin without its KSP, wrapper incompatible with AGP).
- Movement follows the mode: conservative takes the smallest bump satisfying
  the requirement; realistic/optimistic take latest per the mode table.
- Fetch each compat table/release note once; share results across stages.
- Preserve dependency pin comments and KSP's pair syntax — they are the
  environment's constraint memory.
- The lowest min-SDK flavor always bounds the dependency stage; eliminating
  it via a toolchain choice is a product decision to surface, not imply.
- A combined bump ships only when every stage is consistent — stop and
  report if a level forces an unplanned change (higher JDK, new AGP major).

---

## Output contract

Deliver together:

- The per-mode estimate from "Decide the mode first": changes-and-risks for
  all three modes + the chosen one.
- Environment version table: tool/component | current | candidate |
  constraints | decision (+ reason).
- Behavioral-change report for the SDK bump (MUST / verify / no impact),
  covering every SDK level between old and new targetSdk.
- Verification log: config pass, unit-test task for the base minSdk flavor,
  lint.

## Reminders

- Raising the minSdk floor sheds users on old devices — the highest-impact
  decision in an update; a requirement-driven bump starts from the floor
  change and moves the toolchain only as far as needed.
- Order: runtime/toolchain caps everything; dependencies come last.
- Sources of truth, not memory: developer.android.com for behavioral changes,
  dependency release notes for min-SDK floors, compat tables + build scripts
  for current/candidate versions.
- No websearch — the pages in "Startup: current state + pages to fetch" are
  the only sources.
- Surface behavior that activates only after the bump (edge-to-edge,
  predictive back, orientation/large-screen) — shipping it unaddressed is a
  decision, not an oversight.
- Re-audit every SDK level in between when jumping more than one version.
