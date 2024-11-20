# based on FindZLIB.cmake

set(_UNRAR_SEARCHES)

# Search UNRAR_ROOT first if it is set.
if(UNRAR_ROOT)
  set(_UNRAR_SEARCH_ROOT PATHS ${UNRAR_ROOT} NO_DEFAULT_PATH)
  list(APPEND _UNRAR_SEARCHES _UNRAR_SEARCH_ROOT)
endif()

# Normal search.
set(_UNRAR_x86 "(x86)")
set(_UNRAR_SEARCH_NORMAL
    PATHS "$ENV{ProgramFiles}/unrar"
          "$ENV{ProgramFiles${_UNRAR_x86}}/unrar")
unset(_UNRAR_x86)
list(APPEND _UNRAR_SEARCHES _UNRAR_SEARCH_NORMAL)

set(UNRAR_NAMES unrar)
set(UNRAR_NAMES_DEBUG unrar)

# Try each search configuration.
foreach(search ${_UNRAR_SEARCHES})
  find_path(UNRAR_INCLUDE_DIR_UNRAR_H NAMES unrar.h ${${search}} PATH_SUFFIXES include unrar)
endforeach()
if(UNRAR_INCLUDE_DIR_UNRAR_H)
  set(RAR_HDR_UNRAR_H 1)
  set(UNRAR_INCLUDE_DIR ${UNRAR_INCLUDE_DIR_UNRAR_H})
else()
  foreach(search ${_UNRAR_SEARCHES})
    find_path(UNRAR_INCLUDE_DIR_DLL_HPP NAMES dll.hpp ${${search}} PATH_SUFFIXES include unrar)
  endforeach()
  if(UNRAR_INCLUDE_DIR_DLL_HPP)
    set(RAR_HDR_DLL_HPP 1)
    set(UNRAR_INCLUDE_DIR ${UNRAR_INCLUDE_DIR_DLL_HPP})
  endif()
endif()

# Allow UNRAR_LIBRARY to be set manually, as the location of the unrar library
if(NOT UNRAR_LIBRARY)
  foreach(search ${_UNRAR_SEARCHES})
    find_library(UNRAR_LIBRARY_RELEASE NAMES ${UNRAR_NAMES} NAMES_PER_DIR ${${search}} PATH_SUFFIXES lib)
    find_library(UNRAR_LIBRARY_DEBUG NAMES ${UNRAR_NAMES_DEBUG} NAMES_PER_DIR ${${search}} PATH_SUFFIXES lib)
  endforeach()

  include(${CMAKE_ROOT}/Modules/SelectLibraryConfigurations.cmake)
  select_library_configurations(UNRAR)
endif()

unset(UNRAR_NAMES)
unset(UNRAR_NAMES_DEBUG)

mark_as_advanced(UNRAR_INCLUDE_DIR UNRAR_INCLUDE_DIR_UNRAR_H UNRAR_INCLUDE_DIR_DLL_HPP)

if(UNRAR_INCLUDE_DIR AND EXISTS "${UNRAR_INCLUDE_DIR}/version.hpp")
    file(STRINGS "${UNRAR_INCLUDE_DIR}/version.hpp" UNRAR_H REGEX "^#define RARVER_.*$")

    string(REGEX REPLACE "^.*RARVER_MAJOR *([0-9]+).*$" "\\1" UNRAR_VERSION_MAJOR "${UNRAR_H}")
    string(REGEX REPLACE "^.*RARVER_MINOR *([0-9]+).*$" "\\1" UNRAR_VERSION_MINOR "${UNRAR_H}")
    string(REGEX REPLACE "^.*RARVER_BETA *([0-9]+).*$" "\\1" UNRAR_VERSION_PATCH "${UNRAR_H}")
    set(UNRAR_VERSION_STRING "${UNRAR_VERSION_MAJOR}.${UNRAR_VERSION_MINOR}.${UNRAR_VERSION_PATCH}")

    set(UNRAR_MAJOR_VERSION "${UNRAR_VERSION_MAJOR}")
    set(UNRAR_MINOR_VERSION "${UNRAR_VERSION_MINOR}")
    set(UNRAR_PATCH_VERSION "${UNRAR_VERSION_PATCH}")
endif()

include(${CMAKE_ROOT}/Modules/FindPackageHandleStandardArgs.cmake)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(UNRAR REQUIRED_VARS UNRAR_LIBRARY UNRAR_INCLUDE_DIR
                                        VERSION_VAR UNRAR_VERSION_STRING)

if(UNRAR_FOUND)
    set(UNRAR_INCLUDE_DIRS ${UNRAR_INCLUDE_DIR})

    if(NOT UNRAR_LIBRARIES)
      set(UNRAR_LIBRARIES ${UNRAR_LIBRARY})
    endif()

    if(NOT TARGET UNRAR::UNRAR)
      add_library(UNRAR::UNRAR UNKNOWN IMPORTED)
      set_target_properties(UNRAR::UNRAR PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${UNRAR_INCLUDE_DIRS}")

      if(UNRAR_LIBRARY_RELEASE)
        set_property(TARGET UNRAR::UNRAR APPEND PROPERTY
          IMPORTED_CONFIGURATIONS RELEASE)
        set_target_properties(UNRAR::UNRAR PROPERTIES
          IMPORTED_LOCATION_RELEASE "${UNRAR_LIBRARY_RELEASE}")
      endif()

      if(UNRAR_LIBRARY_DEBUG)
        set_property(TARGET UNRAR::UNRAR APPEND PROPERTY
          IMPORTED_CONFIGURATIONS DEBUG)
        set_target_properties(UNRAR::UNRAR PROPERTIES
          IMPORTED_LOCATION_DEBUG "${UNRAR_LIBRARY_DEBUG}")
      endif()

      if(NOT UNRAR_LIBRARY_RELEASE AND NOT UNRAR_LIBRARY_DEBUG)
        set_property(TARGET UNRAR::UNRAR APPEND PROPERTY
          IMPORTED_LOCATION "${UNRAR_LIBRARY}")
      endif()
    endif()
endif()
