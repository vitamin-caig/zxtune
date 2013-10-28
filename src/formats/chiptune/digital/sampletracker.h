/*
Abstract:
  SampleTracker format description

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef FORMATS_CHIPTUNE_SAMPLETRACKER_H_DEFINED
#define FORMATS_CHIPTUNE_SAMPLETRACKER_H_DEFINED

//local includes
#include "digital.h"
//library includes
#include <formats/chiptune.h>

namespace Formats
{
  namespace Chiptune
  {
    namespace SampleTracker
    {
      typedef Digital::Builder Builder;

      Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
    }
  }
}

#endif //FORMATS_CHIPTUNE_SAMPLETRACKER_H_DEFINED
