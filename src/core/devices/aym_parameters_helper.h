/*
Abstract:
  AYM parameters helper

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef __AYM_PARAMETERS_HELPER_H_DEFINED__
#define __AYM_PARAMETERS_HELPER_H_DEFINED__

//common includes
#include <parameters.h>
//library includes
#include <core/freq_tables.h>
//std includes
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

#endif //__AYM_PARAMETERS_HELPER_H_DEFINED__
