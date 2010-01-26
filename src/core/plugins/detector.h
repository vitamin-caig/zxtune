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

#include <types.h>

#include <string>

namespace ZXTune
{
  //Pattern format
  //xx - match byte (hex)
  //? - any byte
  //+xx+ - skip xx bytes (dec)
  bool DetectFormat(const uint8_t* data, std::size_t size, const std::string& pattern);

  struct DetectFormatChain
  {
    const std::string PlayerFP;
    const std::size_t PlayerSize;
  };
}

#endif //__CORE_PLUGINS_DETECTOR_H_DEFINED__
