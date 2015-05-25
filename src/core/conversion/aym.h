/**
*
* @file
*
* @brief  AYM-base conversion details
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <core/plugin_attrs.h>

namespace Module
{
  namespace AYM
  {
    inline uint_t GetSupportedFormatConvertors()
    {
      using namespace ZXTune::Capabilities::Module::Conversion;
      return PSG | ZX50 | AYDUMP | FYM;
    }
  }

  namespace Vortex
  {
    inline uint_t GetSupportedFormatConvertors()
    {
      return 0;
    }
  }
}
