How to build ZXTune
===================

Prerequisites
-------------

- `make` tool
- `boost 1.48+` library
- `qt 4.8+` library (only for `zxtune-qt` application)
- `GCC` toolkit for linux
- `MINGW` toolkit or
- `Microsoft Visual C++ 13.0 Express` with `DirectX SDK` for windows

Environment setup
-----------------

All the build settings specified via `make ... <propname>=<propvalue>` can be stored in optional file `variables.mak` in the project's root and will be used for all builds.
For windows optional file `variables.bat` can be used to setup environment for build shell provided by `shell.bat`

Possible targets
----------------

Location of `Makefile` files of main locations are `apps/zxtune123`, `apps/zxtune-qt` and `apps/xtractor` for `zxtune123`,`zxtune-qt` and `xtractor` respectively.

Build results
-------------

In case of successfull build resulting binaries will be located at `bin` folder with some possible subfolders according to build modes.

Building for linux with system boost/qt libraries installed
-----------------------------------------------------------

Run:
```sh
make system.zlib=1 release=1 -C <path to Makefile>
```

Building in windows environment
===============================

Common steps for mingw/msvc building are:
- choose some directory for prebuilt environment (`prebuilt.dir` in `variables.mak`)
- choose `x86` or `x86_64` architecture (`$(arch)` variable mentioned below)
- optionally create `variables.bat` to set up `make` tool availability

After performing particular environment setup described above, use `shell.bat` to get console session with required parameters set. To start building run:
```sh
make release=1 -C <path to Makefile>
```

Building for mingw
------------------

- download `boost-1.55.0.zip` and `boost-1.55.0-mingw-$(arch).zip` files from http://drive.google.com/folderview?id=0BzfWZ2kQHJsGUG5FNG5FbGpQaUE and unpack it to `$(prebuilt.dir)`
- download `qt-4.8.5-mingw-$(arch).zip` file from http://drive.google.com/folderview?id=0BzfWZ2kQHJsGRWJfRFFXR
- choose some directory for toolchain (`toolchains.root` in `variables.mak`)
- download `mingw` from http://sourceforge.net/projects/mingwbuilds/files/host-windows/releases/4.8.1/ , thread-posix/sjlj and unpack it so that `$(toolchains.root)/Mingw/bin` will contain all the toolchain's binaries.

Building for windows
--------------------

- choose some directory for prebuilt environment (`prebuilt.dir` in `variables.mak`)
- choose `x86` or `x86_64` architecture (`$(arch)` variable)
- download `boost-1.55.0.zip` and `boost-1.55.0-windows-$(arch).zip` files from http://drive.google.com/folderview?id=0BzfWZ2kQHJsGUG5FNG5FbGpQaUE and unpack it to `$(prebuilt.dir)`
- download `qt-4.8.5-windows-$(arch).zip` file from http://drive.google.com/folderview?id=0BzfWZ2kQHJsGRWJfRFFXR

FAQ
===

Why so cumbersome?
------------------

To support all the features required for successfull project support. BTW, ZXTune is quite complex end-user product, so optimizing it for other developers is not a goal.

Why not to use ${BUILDSYSTEMNAME} instead of archaic `make`?
------------------------------------------------------------

Short answer- because I'm fully satisfied by the current one and don't want to waste time without any profit.
If you are advanced in ${BUILDSYSTEMNAME} and want to introduce it in ZXTune project, please contact with maintainer to discuss requirements and possible pitfalls.
