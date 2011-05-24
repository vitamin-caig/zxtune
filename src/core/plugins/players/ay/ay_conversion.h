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

    class ConversionFactory
    {
    public:
      virtual ~ConversionFactory() {}

      virtual Information::Ptr GetInformation() const = 0;
      virtual Parameters::Accessor::Ptr GetProperties() const = 0;
      virtual Renderer::Ptr CreateRenderer(Devices::AYM::Chip::Ptr chip) const = 0;
    };
    //! @brief Simple helper for conversion to AYM-related formats
    //! @param creator Function to create player based on specified device
    //! @param param Input convertion parameter
    //! @param dst Destination data
    //! @param result Result state
    //! @return true if parameter is processed
    bool ConvertAYMFormat(const Conversion::Parameter& spec, const ConversionFactory& factory, Dump& dst, Error& result);

    //! @brief Mask for supported AYM-related formats
    uint_t GetSupportedAYMFormatConvertors();

    //! @brief Simple helper for conversion to Vortex-related formats
    //! @param data Source data for Vortex track
    //! @param info %Module information
    //! @param param Input convertion parameter
    //! @param version Compatibility version
    //! @param freqTable Frequency table
    //! @param dst Destination data
    //! @param result Result state
    //! @return true if parameter is processed
    bool ConvertVortexFormat(const Vortex::Track::ModuleData& data, const Information& info, const Parameters::Accessor& props, const Conversion::Parameter& param,
      uint_t version, const String& freqTable,
      Dump& dst, Error& result);

    //! @brief Mask for supported Vortex-related formats
    uint_t GetSupportedVortexFormatConvertors();
  }
}

#endif //__CORE_PLUGINS_PLAYERS_AY_CONVERSION_H_DEFINED__
