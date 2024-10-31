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

#include <string_view.h>

namespace Module
{
  //@{
  //! @name Base constants

  //! Delimiter for subpath components
  constexpr auto SUBPATH_DELIMITER = '/';

  //! Delimiter for 'Container' attribute components
  constexpr auto CONTAINERS_DELIMITER = '>';
  //@}

  //@{
  //! @name Built-in attributes

  //! %Module type attribute @see PluginInformation#Id
  constexpr auto ATTR_TYPE = "Type"sv;
  //! %Module title
  constexpr auto ATTR_TITLE = "Title"sv;
  //! %Module author
  constexpr auto ATTR_AUTHOR = "Author"sv;
  //! %Module program produced at
  constexpr auto ATTR_PROGRAM = "Program"sv;
  //! %Module computer
  constexpr auto ATTR_COMPUTER = "Computer"sv;
  //! %Module creating date
  constexpr auto ATTR_DATE = "Date"sv;
  //! %Module comment
  constexpr auto ATTR_COMMENT = "Comment"sv;
  //! Internal format version 10 * major + minor
  constexpr auto ATTR_VERSION = "Version"sv;
  //! Strings (e.g. from sample names)
  constexpr auto ATTR_STRINGS = "Strings"sv;
  //! Platform id
  constexpr auto ATTR_PLATFORM = "Platform"sv;
  //! Covert art
  constexpr auto ATTR_PICTURE = "Picture"sv;
  //@}

  //@{
  //! @name Storage-related attributes

  //! Raw module data crc32 checksum
  constexpr auto ATTR_CRC = "CRC"sv;
  //! Constant  module data crc32 checksum
  constexpr auto ATTR_FIXEDCRC = "FixedCRC"sv;
  //! Raw module size in bytes
  constexpr auto ATTR_SIZE = "Size"sv;
  //@}

  //@{
  //! @name Named storage-related attributes

  //! %Module containers chain
  constexpr auto ATTR_CONTAINER = "Container"sv;
  //! %Module path in top-level data dump
  constexpr auto ATTR_SUBPATH = "Subpath"sv;
  //! Module's input data filename extension
  constexpr auto ATTR_EXTENSION = "Extension"sv;
  //! Module's input data filename
  constexpr auto ATTR_FILENAME = "Filename"sv;
  //! Path of module's input data
  constexpr auto ATTR_PATH = "Path"sv;
  //! Full path of module including path of input data and subpath
  constexpr auto ATTR_FULLPATH = "Fullpath"sv;
  //@}

  //@{
  //! @name Runtime attributes

  //! Current module position
  constexpr auto ATTR_CURRENT_POSITION = "CurPosition"sv;
  //! Current module pattern
  constexpr auto ATTR_CURRENT_PATTERN = "CurPattern"sv;
  //! Current module line
  constexpr auto ATTR_CURRENT_LINE = "CurLine"sv;
  //@}
}  // namespace Module
