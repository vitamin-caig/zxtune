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

//std includes
#include <string>

namespace Module
{
  //@{
  //! @name Built-in attributes

  //! %Module type attribute @see PluginInformation#Id
  const std::string ATTR_TYPE("Type");
  //! %Module title
  const std::string ATTR_TITLE("Title");
  //! %Module author
  const std::string ATTR_AUTHOR("Author");
  //! %Module program produced at
  const std::string ATTR_PROGRAM("Program");
  //! %Module computer (hardware platform)
  const std::string ATTR_COMPUTER("Computer");
  //! %Module creating date
  const std::string ATTR_DATE("Date");
  //! %Module comment
  const std::string ATTR_COMMENT("Comment");
  //! Internal format version 10 * major + minor
  const std::string ATTR_VERSION("Version");
  //@}

  //@{
  //! @name Storage-related attributes

  //! Raw module data crc32 checksum
  const std::string ATTR_CRC("CRC");
  //! Constant (without any headers) module data crc32 checksum
  const std::string ATTR_FIXEDCRC("FixedCRC");
  //! Raw module size in bytes
  const std::string ATTR_SIZE("Size");
  //@}

  //@{
  //! @name Named storage-related attributes

  //! %Module containers chain
  const std::string ATTR_CONTAINER("Container");
  //! %Module path in top-level data dump
  const std::string ATTR_SUBPATH("Subpath");
  //! Module's input data filename extension
  const std::string ATTR_EXTENSION("Extension");
  //! Module's input data filename
  const std::string ATTR_FILENAME("Filename");
  //! Path of module's input data
  const std::string ATTR_PATH("Path");
  //! Full path of module including path of input data and subpath
  const std::string ATTR_FULLPATH("Fullpath");
  //@}

  //@{
  //! @name Runtime attributes

  //! Current module position
  const std::string ATTR_CURRENT_POSITION("CurPosition");
  //! Current module pattern
  const std::string ATTR_CURRENT_PATTERN("CurPattern");
  //! Current module line
  const std::string ATTR_CURRENT_LINE("CurLine");
}
