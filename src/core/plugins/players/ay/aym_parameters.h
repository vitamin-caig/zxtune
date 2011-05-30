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
#include <devices/aym.h>

namespace ZXTune
{
  namespace AYM
  {
    Devices::AYM::ChipParameters::Ptr CreateChipParameters(Parameters::Accessor::Ptr params);

    //layout mode
    enum LayoutType
    {
      LAYOUT_ABC = 0,
      LAYOUT_ACB = 1,
      LAYOUT_BAC = 2,
      LAYOUT_BCA = 3,
      LAYOUT_CAB = 4,
      LAYOUT_CBA = 5,

      LAYOUT_LAST
    };

    class TrackParameters
    {
    public:
      typedef boost::shared_ptr<const TrackParameters> Ptr;

      virtual ~TrackParameters() {}

      virtual const Module::FrequencyTable& FreqTable() const = 0;
      virtual LayoutType Layout() const = 0;

      static Ptr Create(Parameters::Accessor::Ptr params);
    };
  }
}

#endif //__AYM_PARAMETERS_HELPER_H_DEFINED__
