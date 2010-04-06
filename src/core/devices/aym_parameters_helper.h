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

    // Performs aym-related parameters applying
    class ParametersHelper
    {
    public:
      typedef std::auto_ptr<ParametersHelper> Ptr;

      virtual ~ParametersHelper() {}

      //called with new parameters set
      virtual void SetParameters(const Parameters::Map& params) = 0;

      //frequency table according to parameters
      virtual const Module::FrequencyTable& GetFreqTable() const = 0;
      //initial data chunk according to parameters
      virtual void GetDataChunk(DataChunk& dst) const = 0;

      static Ptr Create(const String& defaultFreqTable);
    };
  }
}

#endif //__CORE_PLUGINS_PLAYERS_AYM_PARAMETERS_HELPER_H_DEFINED__
