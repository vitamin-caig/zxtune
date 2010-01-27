/*
Abstract:
  Conversion helpers

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef __CORE_PLUGINS_PLAYERS_CONVERT_HELPERS_H_DEFINED__
#define __CORE_PLUGINS_PLAYERS_CONVERT_HELPERS_H_DEFINED__

#include <core/module_player.h>
#include <core/devices/aym.h>

#include <boost/function.hpp>

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
    //! @param creator Function to create player based on specified device
    //! @param param Input convertion parameter
    //! @param dst Destination data
    //! @param result Result state
    //! @return true if parameter is processed
    bool ConvertAYMFormat(const boost::function<Player::Ptr(AYM::Chip::Ptr)>& creator, const Conversion::Parameter& param, Dump& dst, Error& result);
  }
}

#endif //__CORE_PLUGINS_PLAYERS_CONVERT_HELPERS_H_DEFINED__
