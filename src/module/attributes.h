/**
 *
 * @file
 *
 * @brief  Module attributes names
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <types.h>

namespace Module
{
  //@{
  //! @name Base constants

  //! Delimiter for subpath components
  constexpr const Char SUBPATH_DELIMITER = '/';

  //! Delimiter for 'Container' attribute components
  constexpr const Char CONTAINERS_DELIMITER = '>';
  //@}

  //@{
  //! @name Built-in attributes

  //! %Module type attribute @see PluginInformation#Id
  constexpr const auto ATTR_TYPE = "Type"_sv;
  //! %Module title
  constexpr const auto ATTR_TITLE = "Title"_sv;
  //! %Module author
  constexpr const auto ATTR_AUTHOR = "Author"_sv;
  //! %Module program produced at
  constexpr const auto ATTR_PROGRAM = "Program"_sv;
  //! %Module computer
  constexpr const auto ATTR_COMPUTER = "Computer"_sv;
  //! %Module creating date
  constexpr const auto ATTR_DATE = "Date"_sv;
  //! %Module comment
  constexpr const auto ATTR_COMMENT = "Comment"_sv;
  //! Internal format version 10 * major + minor
  constexpr const auto ATTR_VERSION = "Version"_sv;
  //! Strings (e.g. from sample names)
  constexpr const auto ATTR_STRINGS = "Strings"_sv;
  //! Platform id
  constexpr const auto ATTR_PLATFORM = "Platform"_sv;
  //! Covert art
  constexpr const auto ATTR_PICTURE = "Picture"_sv;
  //@}

  //@{
  //! @name Storage-related attributes

  //! Raw module data crc32 checksum
  constexpr const auto ATTR_CRC = "CRC"_sv;
  //! Constant  module data crc32 checksum
  constexpr const auto ATTR_FIXEDCRC = "FixedCRC"_sv;
  //! Raw module size in bytes
  constexpr const auto ATTR_SIZE = "Size"_sv;
  //@}

  //@{
  //! @name Named storage-related attributes

  //! %Module containers chain
  constexpr const auto ATTR_CONTAINER = "Container"_sv;
  //! %Module path in top-level data dump
  constexpr const auto ATTR_SUBPATH = "Subpath"_sv;
  //! Module's input data filename extension
  constexpr const auto ATTR_EXTENSION = "Extension"_sv;
  //! Module's input data filename
  constexpr const auto ATTR_FILENAME = "Filename"_sv;
  //! Path of module's input data
  constexpr const auto ATTR_PATH = "Path"_sv;
  //! Full path of module including path of input data and subpath
  constexpr const auto ATTR_FULLPATH = "Fullpath"_sv;
  //@}

  //@{
  //! @name Runtime attributes

  //! Current module position
  constexpr const auto ATTR_CURRENT_POSITION = "CurPosition"_sv;
  //! Current module pattern
  constexpr const auto ATTR_CURRENT_PATTERN = "CurPattern"_sv;
  //! Current module line
  constexpr const auto ATTR_CURRENT_LINE = "CurLine"_sv;
  //@}
}  // namespace Module
