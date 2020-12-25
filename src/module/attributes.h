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

//common includes
#include <types.h>

namespace Module
{
  //@{
  //! @name Built-in attributes

  //! %Module type attribute @see PluginInformation#Id
  extern const String ATTR_TYPE;
  //! %Module title
  extern const String ATTR_TITLE;
  //! %Module author
  extern const String ATTR_AUTHOR;
  //! %Module program produced at
  extern const String ATTR_PROGRAM;
  //! %Module computer 
  extern const String ATTR_COMPUTER;
  //! %Module creating date
  extern const String ATTR_DATE;
  //! %Module comment
  extern const String ATTR_COMMENT;
  //! Internal format version 10 * major + minor
  extern const String ATTR_VERSION;
  //! Strings (e.g. from sample names)
  extern const String ATTR_STRINGS;
  //! Platform id
  extern const String ATTR_PLATFORM;
  //@}

  //@{
  //! @name Storage-related attributes

  //! Raw module data crc32 checksum
  extern const String ATTR_CRC;
  //! Constant  module data crc32 checksum
  extern const String ATTR_FIXEDCRC;
  //! Raw module size in bytes
  extern const String ATTR_SIZE;
  //@}

  //@{
  //! @name Named storage-related attributes

  //! %Module containers chain
  extern const String ATTR_CONTAINER;
  //! %Module path in top-level data dump
  extern const String ATTR_SUBPATH;
  //! Module's input data filename extension
  extern const String ATTR_EXTENSION;
  //! Module's input data filename
  extern const String ATTR_FILENAME;
  //! Path of module's input data
  extern const String ATTR_PATH;
  //! Full path of module including path of input data and subpath
  extern const String ATTR_FULLPATH;
  //@}

  //@{
  //! @name Runtime attributes

  //! Current module position
  extern const String ATTR_CURRENT_POSITION;
  //! Current module pattern
  extern const String ATTR_CURRENT_PATTERN;
  //! Current module line
  extern const String ATTR_CURRENT_LINE;
  //@}
}
