/*
Abstract:
  AYM parameters helper

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef __CORE_PLUGINS_PLAYERS_AYM_PARAMETERS_HELPER_H_DEFINED__
#define __CORE_PLUGINS_PLAYERS_AYM_PARAMETERS_HELPER_H_DEFINED__

#include <parameters.h>
#include <core/freq_tables.h>

#include <memory>

namespace ZXTune
{
  namespace AYM
  {
    struct DataChunk;

    class ParametersHelper
    {
    public:
      typedef std::auto_ptr<ParametersHelper> Ptr;

      virtual ~ParametersHelper() {}

      virtual void SetParameters(const Parameters::Map& params) = 0;

      virtual const Module::FrequencyTable& GetFreqTable() const = 0;
      virtual void GetDataChunk(DataChunk& dst) const = 0;

      static Ptr Create(const String& defaultFreqTable);
    };
  }
}

#endif //__CORE_PLUGINS_PLAYERS_AYM_PARAMETERS_HELPER_H_DEFINED__
