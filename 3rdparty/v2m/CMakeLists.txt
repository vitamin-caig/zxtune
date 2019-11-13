cmake_minimum_required(VERSION 2.8)

project(v2mplayer)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNDEBUG -Os -flto -Wall -fno-asynchronous-unwind-tables -fno-stack-protector -ffunction-sections -fdata-sections -Wl,--gc-sections")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNDEBUG -DRONAN -Os -flto -Wall -fno-asynchronous-unwind-tables -fno-stack-protector -ffunction-sections -fdata-sections -Wl,--gc-sections")

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake)
set(MAJOR_VERSION 0)
set(DATE_VERSION 20180825)
set(V2M_VERSION ${MAJOR_VERSION}.${DATE_VERSION})
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIR})

add_library(v2m
    SHARED
    src/ronan.cpp
    src/v2mplayer.cpp
    src/v2mconv.cpp
    src/synth_core.cpp
    src/sounddef.cpp
)
set_target_properties(v2m PROPERTIES VERSION ${V2M_VERSION} SOVERSION ${MAJOR_VERSION})

add_executable(v2mplayer
    src/tinyplayer.cpp
)
target_link_libraries(v2mplayer
    ${SDL2_LIBRARY}
    v2m
)

#file(GLOB V2M_EXAMPLE v2m/*.v2mz)
set(V2M_EXAMPLE v2m/0test.v2m v2m/Dafunk--breeze.v2mz)

install(TARGETS v2m DESTINATION lib)
install(TARGETS v2mplayer DESTINATION bin)
install(FILES man/man1/v2mplayer.1 DESTINATION share/man/man1)
install(FILES src/libv2.h src/phonemtab.h src/sounddef.h src/synth.h src/types.h src/v2mconv.h src/v2mplayer.h DESTINATION include/v2m)
install(FILES LICENSE README.md doc/AUTHORS doc/CHANGELOG doc/VERSION DESTINATION share/doc/v2mplayer)
install(FILES ${V2M_EXAMPLE} DESTINATION share/doc/v2mplayer/v2m)
