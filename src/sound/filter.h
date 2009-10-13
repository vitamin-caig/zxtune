/*
Abstract:
  Defenition of filtering-related functionality

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef __SOUND_FILTER_H_DEFINED__
#define __SOUND_FILTER_H_DEFINED__

#include "receiver.h"

namespace ZXTune
{
  namespace Sound
  {
    /*
      FIR filter with fixed-point calculations
    */
    SoundConverter::Ptr CreateFIRFilter(const std::vector<signed>& coeffs);

    void CalculateBandpassFilter(unsigned freq, unsigned lowCutoff, unsigned highCutoff, std::vector<signed>& coeffs);
  }
}

#endif //__FILTER_H_DEFINED__
