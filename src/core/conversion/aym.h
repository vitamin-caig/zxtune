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
    const uint_t SupportedFormatConvertors = ZXTune::CAP_CONV_PSG | ZXTune::CAP_CONV_ZX50 | ZXTune::CAP_CONV_AYDUMP | ZXTune::CAP_CONV_FYM;
  }

  namespace Vortex
  {
    const uint_t SupportedFormatConvertors = 0;
  }
}
