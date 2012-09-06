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
    //! @param result Result state
    //! @return true if parameter is processed
    bool ConvertAYMFormat(const AYM::Chiptune& chiptune, const Conversion::Parameter& spec, Parameters::Accessor::Ptr params,
      Dump& dst, Error& result);

    //! @brief Mask for supported AYM-related formats
    uint_t GetSupportedAYMFormatConvertors();

    //! @brief Simple helper for conversion to Vortex-related formats
    //! @param data Source data for Vortex track
    //! @param info %Module information
    //! @param param Input convertion parameter
    //! @param dst Destination data
    //! @param result Result state
    //! @return true if parameter is processed
    bool ConvertVortexFormat(const Vortex::Track::ModuleData& data, const Information& info, const Parameters::Accessor& props, const Conversion::Parameter& param,
      Dump& dst, Error& result);

    //! @brief Mask for supported Vortex-related formats
    uint_t GetSupportedVortexFormatConvertors();

    Error CreateUnsupportedConversionError(Error::LocationRef loc, const Conversion::Parameter& param);
  }
}

#endif //__CORE_PLUGINS_PLAYERS_AY_CONVERSION_H_DEFINED__
