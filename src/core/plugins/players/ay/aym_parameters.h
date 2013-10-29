/**
* 
* @file
*
* @brief  AYM parameters helpers
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <core/freq_tables.h>
#include <core/module_types.h>
#include <devices/aym/chip.h>
#include <parameters/accessor.h>

namespace Module
{
  namespace AYM
  {
    Devices::AYM::ChipParameters::Ptr CreateChipParameters(Parameters::Accessor::Ptr params);

    class TrackParameters
    {
    public:
      typedef boost::shared_ptr<const TrackParameters> Ptr;

      virtual ~TrackParameters() {}

      virtual uint_t Version() const = 0;
      virtual void FreqTable(FrequencyTable& table) const = 0;

      static Ptr Create(Parameters::Accessor::Ptr params);
    };
  }
}
