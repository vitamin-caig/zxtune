# CMake project definition file.
cmake_minimum_required(VERSION 3.3...3.5)
project(libgme)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

include(CMakePushCheckState)
include(CheckCXXSourceCompiles)
include(TestBigEndian)
include(GNUInstallDirs)
include(CTest)

file(READ "gme/gme.h" GME_GME_H)
string(REGEX MATCH "/\\* Game_Music_Emu ([0-9]+)\\.([0-9]+).([0-9]+) \\*/" RE_GME_VERSION "${GME_GME_H}")
if(NOT RE_GME_VERSION)
    message(FATAL_ERROR "Unable to extract gme version from gme/gme.h")
endif()
set(GME_VERSION_MAJOR "${CMAKE_MATCH_1}")
set(GME_VERSION_MINOR "${CMAKE_MATCH_2}")
set(GME_VERSION_PATCH "${CMAKE_MATCH_3}")
set(GME_VERSION "${GME_VERSION_MAJOR}.${GME_VERSION_MINOR}.${GME_VERSION_PATCH}")

if ("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
    set(GME_IS_ROOT TRUE)
else()
    set(GME_IS_ROOT FALSE)
endif()

# Default emulators to build (all of them! ;)
option(USE_GME_AY "Enable support for Spectrum ZX music emulation" ON)
option(USE_GME_GBS "Enable support for Game Boy music emulation" ON)
option(USE_GME_GYM "Enable Sega MegaDrive/Genesis music emulation" ON)
option(USE_GME_HES "Enable PC Engine/TurboGrafx-16 music emulation" ON)
option(USE_GME_KSS "Enable MSX or other Z80 systems music emulation" ON)
option(USE_GME_NSF "Enable NES NSF music emulation" ON)
option(USE_GME_NSFE "Enable NES NSFE and NSF music emulation" ON)
option(USE_GME_SAP "Enable Atari SAP music emulation" ON)
option(USE_GME_SPC "Enable SNES SPC music emulation" ON)
option(USE_GME_VGM "Enable Sega VGM/VGZ music emulation" ON)

option(GME_SPC_ISOLATED_ECHO_BUFFER "Enable isolated echo buffer on SPC emulator to allow correct playing of \"dodgy\" SPC files made for various ROM hacks ran on ZSNES" OFF)
option(GME_ZLIB "Enable GME to support compressed sound formats" ON)

set(GME_YM2612_EMU "Nuked" CACHE STRING "Which YM2612 emulator to use: \"Nuked\" (LGPLv2.1+), \"MAME\" (GPLv2+), or \"GENS\" (LGPLv2.1+)")
set(GME_YM2612_EMU_CHOICES "Nuked;MAME;GENS")
set_property(CACHE GME_YM2612_EMU PROPERTY STRINGS "${GME_YM2612_EMU_CHOICES}")

if(USE_GME_NSFE AND NOT USE_GME_NSF)
    message(STATUS "NSFE support requires NSF, enabling NSF support.")
    set(USE_GME_NSF 1 CACHE BOOL "Enable NES NSF music emulation" FORCE)
endif()

if(DEFINED BUILD_SHARED_LIBS)
    set(GME_BUILD_SHARED_DEFAULT ${BUILD_SHARED_LIBS})
else()
    set(GME_BUILD_SHARED_DEFAULT ON)
endif()

option(GME_BUILD_SHARED "Build shared library (set to OFF for static library)" ${GME_BUILD_SHARED_DEFAULT})
option(GME_BUILD_FRAMEWORK "Build framework instead of dylib on macOS" ${BUILD_FRAMEWORK})

option(GME_ENABLE_UBSAN "Enable Undefined Behavior Sanitizer error-checking" ${ENABLE_UBSAN})
option(GME_BUILD_TESTING "Build demo tests" ${BUILD_TESTING})
option(GME_BUILD_EXAMPLES "Add example project build rules" ${GME_IS_ROOT})

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

test_big_endian(WORDS_BIGENDIAN)

if(MSVC)
    string(REGEX REPLACE "/W[3|4]" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
    string(REGEX REPLACE "/W[3|4]" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    add_compile_options(/W2)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -W -Wextra)
endif()

string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_LOWER)

# Enable debug asserts when build type is debug
if(CMAKE_BUILD_TYPE_LOWER STREQUAL "debug")
    if(CMAKE_COMPILER_IS_GNUCXX)
        add_definitions(-D_GLIBCXX_DEBUG=1 -D_GLIBCXX_ASSERTIONS=1)
    endif()
endif()

# Check for GCC/Clang "visibility" support.
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang" AND GME_ENABLE_UBSAN)
    # GCC needs -static-libubsan
    if(NOT GME_BUILD_SHARED AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined -static-libubsan")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
    endif()
endif()

# Shared library defined here
add_subdirectory(gme)

# Add example project build rules
if (GME_BUILD_EXAMPLES)
    # EXCLUDE_FROM_ALL adds build rules but keeps it out of default build
    add_subdirectory(player EXCLUDE_FROM_ALL)
    add_subdirectory(demo EXCLUDE_FROM_ALL)
endif()
