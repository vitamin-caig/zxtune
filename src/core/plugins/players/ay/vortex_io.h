/*
Abstract:
  Vortex IO functions declaations

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __CORE_PLUGINS_PLAYERS_VORTEX_IO_H_DEFINED__
#define __CORE_PLUGINS_PLAYERS_VORTEX_IO_H_DEFINED__

//local includes
#include "vortex_base.h"
//common includes
#include <error.h>
//std includes
#include <string>

namespace ZXTune
{
  namespace Module
  {
    class ModuleProperties;

    namespace Vortex
    {
      // TXT input-output
      // version is minor value
      Error ConvertFromText(const std::string& text, Vortex::Track::ModuleData& data,
        ModuleProperties& resProps,
        uint_t& version, String& freqTable);
      std::string ConvertToText(const Vortex::Track::ModuleData& data, const Information& info, const Parameters::Accessor& props, uint_t version, const String& freqTable);
    }
  }
}

#endif //__CORE_PLUGINS_PLAYERS_VORTEX_IO_H_DEFINED__
