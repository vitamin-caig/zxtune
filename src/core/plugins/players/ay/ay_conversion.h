/*
Abstract:
  AY-based conversion helpers

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __CORE_PLUGINS_PLAYERS_AY_CONVERSION_H_DEFINED__
#define __CORE_PLUGINS_PLAYERS_AY_CONVERSION_H_DEFINED__

//local includes
#include "ay_base.h"
#include "vortex_base.h"
#include "core/plugins/players/renderer.h"

//forward declarations
class Error;

namespace ZXTune
{
  namespace Module
  {
    namespace Conversion
    {
      struct Parameter;
    }

    //! @brief Simple helper for conversion to AYM-related formats
    //! @param spec Input convertion parameter
    Binary::Data::Ptr ConvertAYMFormat(const AYM::Holder& holder, const Conversion::Parameter& spec, Parameters::Accessor::Ptr params);

    //! @brief Mask for supported AYM-related formats
    uint_t GetSupportedAYMFormatConvertors();

    //! @brief Mask for supported Vortex-related formats
    uint_t GetSupportedVortexFormatConvertors();

    Error CreateUnsupportedConversionError(Error::LocationRef loc, const Conversion::Parameter& param);
  }
}

#endif //__CORE_PLUGINS_PLAYERS_AY_CONVERSION_H_DEFINED__
