Building with CMake:
--------------------------------------------------------

If you only want to use this library as a part of an emulator or other 
project and link it statically and your project uses CMake
then skip this section and go directly to "Using in your CMake-driven project".

CMake (http://www.cmake.org/) is required to build the library.
If you use a popular Linux distribuion chances are that you already have 
it if not installed than in the stock repository.
Out-of-source-tree building is recommended.

To build on Unix and Unix-like environments (GCC,mingw,cygwin,DJGPP):
    mkdir build
    cd build
    cmake ..
To build everything (static and shared z80ex and z80ex_dasm libraries):
    gmake
To build only specific libraries and flavors:
    gmake z80ex (shared z80ex)
    gmake z80ex-static (static z80ex)
    gmake z80ex_dasm (shared dasm)
    gmake z80ex_dasm-static (static z80ex_dasm)

To build using MSVC (Visual Studio):
    - create a folder inside the source tree (e.g. "build")
    - start cmd.exe and go to the created folder and issue:
        cmake ..
    - double-click z80ex.sln to open it in Visual Studio
    - build ALL_BUILD or only necessary target(s) in Visual Studio
    (choose Release/Debug configuration as required)

You may override some build parameters by using ccmake or cmake-gui or other means CMake provides (see CMake documentation and relevant man pages):
* OPSTEP_FAST_AND_ROUGH -- fast and rough opcode step emulation mode (0 - off, 1 - on)
	when this mode is on, timings of internal I/O operations are ignored,
	and tstate callback feature is disabled


Installing (UNIX/MacOS X):
--------------------------

issue "gmake install" as superuser.
(default install prefix is /usr/local, which may be changed using ccmake or 
cmake-gui)
then do "/sbin/ldconfig" as superuser to update DSO links and cache


Using in your CMake-driven project:
-----------------------------------

If your project uses CMake here is an example (copy these lines into your
project's CMakeLists.txt):

    set (Z80EX_PATH "${PROJECT_SOURCE_DIR}/../z80ex"
             CACHE PATH "Path to Z80Ex library")
    include_directories("${Z80EX_PATH}/include")
    set (Z80EX_BINARY_DIR "${PROJECT_BINARY_DIR}/lib_z80ex")
    make_directory (${Z80EX_BINARY_DIR})
    set (Z80EX_STATIC_ONLY true)
    add_subdirectory (${Z80EX_PATH} ${Z80EX_BINARY_DIR})
    link_directories (${Z80EX_BINARY_DIR})
    target_link_libraries (my_cool_emulator_exexutable z80ex-static z80ex_dasm-static)
