/*
Abstract:
  AYM parameters helper

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __AYM_PARAMETERS_HELPER_H_DEFINED__
#define __AYM_PARAMETERS_HELPER_H_DEFINED__

//common includes
#include <parameters.h>
//library includes
#include <core/freq_tables.h>
#include <core/module_types.h>
#include <devices/aym/chip.h>

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

#endif //__AYM_PARAMETERS_HELPER_H_DEFINED__
