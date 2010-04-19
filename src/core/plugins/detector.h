/*
Abstract:
  Format detector interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef __CORE_PLUGINS_DETECTOR_H_DEFINED__
#define __CORE_PLUGINS_DETECTOR_H_DEFINED__

//common includes
#include <types.h>
//std includes
#include <string>

namespace ZXTune
{
  //Pattern format
  //xx - match byte (hex)
  //? - any byte
  //+xx+ - skip xx bytes (dec)
  bool DetectFormat(const uint8_t* data, std::size_t size, const std::string& pattern);
}

#endif //__CORE_PLUGINS_DETECTOR_H_DEFINED__
