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
- Shared Gradle logic lives at repo root: `make/android/{project,android,nativelibs}.gradle` - read before changing build config.
- `Makefile` at this directory just proxies every target to `./gradlew` (`make foo` == `./gradlew foo`).

## Build prerequisites (missing in repo, required for ANY invocation)

Two local, never-committed files are required before Gradle can even configure:

1. `gradle.properties` - MUST contain `android.useAndroidX=true` or configuration fails with an "AndroidX dependencies ... property is not enabled" error.
2. `local.properties` - loaded at configuration time; absence makes every task fail with `...(No such file or directory)`. Keys:
   - `sdk.dir`, `ndk.version` (requires a matching NDK installed)
   - `api.root`, `cdn.root`, `proxy.root` - emitted as `BuildConfig` strings and referenced unconditionally by `zxtune/src/main/java/app/zxtune/fs/api/{Api,Cdn,Proxy}.kt`; compilation FAILS without them. Any URL works for compiling/tests; the real values point at the deployed backend.
   - signing passwords for `develop`/`release`/store variants: `key.store.password`, `key.release.password` (fallback `key.alias.password`), `key.upload_google.password`, `key.upload_rustore.password`, `key.fdroid.password` (keystore file `make/android/keystore` IS committed)
   - ABI filters per packaging flavor: `flavors.fat.abifilters`, `flavors.thin.abifilters` (AAB/google+rustore), `flavors.splitted.abifilters`; a flavor with empty filters is silently disabled
   - optional: `key.modarchive`, `build.jni.max_linkers`, `cdn.root`/`proxy.root`/`api.root` overrides
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
