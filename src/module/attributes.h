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
  extern const std::string ATTR_TYPE;
  //! %Module title
  extern const std::string ATTR_TITLE;
  //! %Module author
  extern const std::string ATTR_AUTHOR;
  //! %Module program produced at
  extern const std::string ATTR_PROGRAM;
  //! %Module computer 
  extern const std::string ATTR_COMPUTER;
  //! %Module creating date
  extern const std::string ATTR_DATE;
  //! %Module comment
  extern const std::string ATTR_COMMENT;
  //! Internal format version 10 * major + minor
  extern const std::string ATTR_VERSION;
  //! Strings (e.g. from sample names)
  extern const std::string ATTR_STRINGS;
  //! Platform id
  extern const std::string ATTR_PLATFORM;
  //@}

  //@{
  //! @name Storage-related attributes

  //! Raw module data crc32 checksum
  extern const std::string ATTR_CRC;
  //! Constant  module data crc32 checksum
  extern const std::string ATTR_FIXEDCRC;
  //! Raw module size in bytes
  extern const std::string ATTR_SIZE;
  //@}

  //@{
  //! @name Named storage-related attributes

  //! %Module containers chain
  extern const std::string ATTR_CONTAINER;
  //! %Module path in top-level data dump
  extern const std::string ATTR_SUBPATH;
  //! Module's input data filename extension
  extern const std::string ATTR_EXTENSION;
  //! Module's input data filename
  extern const std::string ATTR_FILENAME;
  //! Path of module's input data
  extern const std::string ATTR_PATH;
  //! Full path of module including path of input data and subpath
  extern const std::string ATTR_FULLPATH;
  //@}

  //@{
  //! @name Runtime attributes

  //! Current module position
  extern const std::string ATTR_CURRENT_POSITION;
  //! Current module pattern
  extern const std::string ATTR_CURRENT_PATTERN;
  //! Current module line
  extern const std::string ATTR_CURRENT_LINE;
  //@}
}
