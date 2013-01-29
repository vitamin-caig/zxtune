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

namespace ZXTune
{
  namespace Module
  {
    const uint_t SupportedAYMFormatConvertors = CAP_CONV_PSG | CAP_CONV_ZX50 | CAP_CONV_AYDUMP | CAP_CONV_FYM;

    const uint_t SupportedVortexFormatConvertors = 0;
  }
}

#endif //CORE_CONVERSION_AYM_H_DEFINED
