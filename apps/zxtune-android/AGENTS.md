# zxtune-android

Android frontend for ZXTune. Native playback engine is built from the C++ sources in `zxtune/src/main/jni/` (a GNU make project at the repo root, NOT Android Gradle).

## Repository layout

- `zxtune/` - the only Gradle module (`app.zxtune`)
  - `zxtune/src/main/java/` - Kotlin + legacy Java code
  - `zxtune/src/main/jni/` - C++ JNI layer; uses repo-wide `make` build, not CMake
  - `zxtune/src/main/aidl/`, `res/`, `AndroidManifest.xml`
  - `zxtune/src/test/` - Robolectric JVM unit tests
  - `zxtune/src/androidTest/` - on-device instrumented tests (need device + real native libs)
  - `zxtune/src/fdroid/` - flavor-specific resources
- Shared Gradle logic lives at repo root: `make/android/{project,android,nativelibs,r8report}.gradle` - read before changing build config.
- `Makefile` at this directory just proxies every target to `./gradlew` (`make foo` == `./gradlew foo`).

## Build prerequisites (required for ANY invocation)

`gradle.properties` is committed and carries the required `android.useAndroidX=true` plus the AGP 9 compatibility opt-outs; keep local overrides there only if needed. The one local, never-committed file that must exist before Gradle can even configure:

- `local.properties` - loaded at configuration time; absence makes every task fail with `...(No such file or directory)`. Keys:
  - `sdk.dir`, `ndk.version` (requires a matching NDK installed)
  - `api.root`, `cdn.root`, `proxy.root` - emitted as `BuildConfig` strings and referenced unconditionally by `zxtune/src/main/java/app/zxtune/fs/api/{Api,Cdn,Proxy}.kt`; compilation FAILS without them. Any URL works for compiling/tests; the real values point at the deployed backend.
  - signing passwords for `develop`/`release`/store variants: `key.store.password`, `key.release.password` (fallback `key.alias.password`), `key.upload_google.password`, `key.upload_rustore.password`, `key.fdroid.password` (keystore file `make/android/keystore` IS committed)
  - ABI filters per packaging flavor: `flavors.fat.abifilters`, `flavors.thin.abifilters` (AAB/google+rustore), `flavors.splitted.abifilters`; a flavor with empty filters is silently disabled
  - optional: `key.modarchive`, `build.jni.max_linkers`, `build.jni.output` (`all`/`error`/`none`, default `all`), `cdn.root`/`proxy.root`/`api.root` overrides
  - the machine's `variables.mak` at the repo root defines `android.ndk` (the NDK root used by the C++ make build).

## Variants and flavors

Two flavor dimensions (see `nativelibs.gradle`):
- `packaging`: `fat` (single APK), `splitted` (per-ABI APKs), `google`/`rustore` (AAB), `fdroid` (applicationIdSuffix `fdroid`)
- `api`: `minsdk16`, `minsdk28`

Only `minsdk16` variants exist for APK flavors; `minsdk28` only for AAB flavors (and not rustore). `develop` build type is only enabled for `fat`. The full variant name is e.g. `fatMinsdk16Develop`.

## Commands

- Unit tests (fast, no device, no native build, no signing): `./gradlew zxtune:testFatMinsdk16DevelopUnitTest --tests <FqTestClass>` (or with `.method`).
  - Fragment tests (`ui/browser/BrowserFragmentTest`, `ui/playlist/PlaylistFragmentTest`) MUST run under the `Develop` build type - the `fragment-testing-manifest` dependency is `developImplementation` only and the merged manifest breaks under Debug.
  - Robolectric `sdk=28` is pinned via `zxtune/src/test/resources/robolectric.properties`.
- All unit tests for a variant: `...testFatMinsdk16DevelopUnitTest`; there is no build-type aggregate task.
- On-device tests need an emulator/device: `./gradlew zxtune:connectedFatMinsdk16DebugAndroidTest` - requires a full native build.
- Lint: `./gradlew lintFatMinsdk16Develop`; failures do not abort (`lintOptions { abortOnError = false }`).

## R8 removal report

`release` and `develop` both run R8. AGP writes `mapping.txt` / `usage.txt` / `seeds.txt` per variant under `pkg/zxtune/outputs/mapping/<variant>/`, and `make/android/r8report.gradle` turns those into a `report.txt` there, classifying every `app.zxtune*` symbol `usage.txt` lists as:

- `RETAINED` - still a member of the output class
- `INLINED` - body folded into a caller, which `mapping.txt` shows as an extra frame of the same output range
- `GONE` - nothing refers to it and it is absent from the output
- `both` - retained at one call site and inlined at another

`usage.txt` has no reason tags, so the verdict comes from the frame groups in `mapping.txt`. Because mapping records frames by name rather than full signature, a removed overload sharing a name with a retained one is flagged as a collision instead of being guessed. The summary tree also lists retained classes that shed members as `removed: ...`, so class-level removal and per-member removal read the same way.

Besides the tree, `report.txt` carries `SUSPECT` signals about the keep rules:

- App classes the keep rules (`seeds.txt`) protected in whole but the output left out entirely - `@Keep` guarantees that did not hold. Only bare class entries count: `Class: <member>` (e.g. AGP's default `-keepclassmembers enum *`) protects one member, not the class, so R8 may legitimately shrink or unbox the class.
- Classes the merged `AndroidManifest.xml` names by string (`application`/`activity`/`activity-alias`/`service`/`provider`/`receiver` `android:name`, and layout `<fragment android:name>`) that the output left out - the manifest is a runtime lookup R8 cannot see, so a miss only shows up as a `ClassNotFoundException` on device. The manifest is auto-discovered from `pkg/zxtune/intermediates/merged_manifests/<variant>/.../AndroidManifest.xml`; pass `--manifest PATH` to override. An empty result (section absent) means every referenced class survived.
- Room database implementations (`*_Impl`) that survived but lost their no-arg constructor. Room resolves `<DatabaseClass>_Impl` by string and instantiates it via `Class.forName(...).getDeclaredConstructor().newInstance()`, so R8 cannot see the reference; Room's own consumer rule keeps the class name but not the constructor. Once it is pruned the `super()` chain collapses and `runInTransaction` bodies are rewritten to `throw null`. The `-keep class * extends androidx.room.RoomDatabase { <init>(); }` rule in `zxtune/proguard-rules.pro` is what protects it; a member-level removal in `usage.txt` is the only trace.

Do NOT treat a member's `:0:0` original source range in `mapping.txt` as a bug: it is normal for any body with no source line (compiler-synthesized `equals`/`hashCode`/`toString`, enum `values`/`valueOf`, lambda/delegation forwarders, and R8-generated `$$ExternalSyntheticLambda`/`$r8$lambda$`/`BUOutline`/bridges), so a healthy release has thousands. The Room shutdown bug (`RoomDatabase.getAutoMigrations`) had the same shape, but the sentinel cannot tell the two apart, so it is not reported.

- Runs automatically after every `minify*WithR8` (`finalizedBy`), with no property to opt out; the summary is printed to stdout. It runs `r8report.py --strict`, so any `SUSPECT` signal fails the build - a stripped class (e.g. a Room keep rule that did not hold) is a runtime crash, not a warning. Fix the keep rules rather than the report.
- Re-run on demand: `./gradlew zxtune:r8ReportFatMinsdk16Release` (or `...Develop`).
- Self-test the classifier: `./gradlew zxtune:r8ReportTest`, or `python3 make/android/test_r8report.py`. Golden fixtures cover multi-frame groups, owner-less frames, residual-signature comments, fields, constructors and missing seeds; live checks (run when a release build exists) also assert every JNI-reached and manifest-referenced class survived.
- Do NOT add `-printusage`/`-printmapping`/`-printconfiguration` to `zxtune/proguard-rules.pro`. Duplicates there land in the module directory, where variants clobber each other, and AGP already emits per-variant copies.

## Native libs are the main cost driver

`assemble*` / `bundle*` / `install*` / `publish*` first run the Gradle `nativeLibs` task, which invokes the repo-wide `make platform=android arch=... android.minsdk=...` build for every configured ABI/minSdk - the whole codec stack, possibly for 4 ABIs. This is slow. For pure Kotlin/Java changes, verify with unit tests instead; only build an APK when the native side matters.

For C++ changes note the root `AGENTS.md` rules; there is also a host-only stub harness built by plain `make` from the repo root: `apps/zxtune-android/zxtune/src/main/jni/stub` (uses the same sources, no NDK).

## Versioning and publishing

- `versionCode`/`versionName` come from `git describe` via `make/version.mak` (called at Gradle configuration time; e.g. `r5101`, `r5101-107-g9669e30d6M`). Missing git tags collapse the version to `develop`/0.
- Publishing tasks (`publishApkFatMinsdk16Release`, per-AAB `publishAab*Release`, aggregate `publicBuild`). Output locations (`pkg/`, `Builds/`) are covered in the root `AGENTS.md`.
- `checkdeps.py` is stale: it references tasks (`publicBuildFatRelease`, `publicBuildWithCrashlytics`) that no longer exist in this build.

## Misc conventions

- New code is Kotlin; many legacy `.java` files remain, keep their style when touching them.
- Kotlin/Java style is defined in the committed `.editorconfig` and enforced by Spotless (`zxtune/build.gradle`). Run `make spotlessCheck` after Kotlin/Java changes; `make spotlessApply` auto-fixes them (only touches files changed vs HEAD, so legacy sources are left alone).
- Logging goes through the `app.zxtune.Log` wrapper, not `android.util.Log`.
- Git commits should start with 'zxtune-android:' prefix for all changes done in app's directory.
