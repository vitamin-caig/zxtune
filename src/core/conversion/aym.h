/*
Abstract:
  AY-based conversion helpers

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef CORE_CONVERSION_AYM_H_DEFINED
#define CORE_CONVERSION_AYM_H_DEFINED

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

#endif //CORE_CONVERSION_AYM_H_DEFINED
