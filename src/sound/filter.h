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

#include <sound/receiver.h>

//forward declarations
class Error;

namespace ZXTune
{
  namespace Sound
  {
    class Filter : public Converter
    {
    public:
      typedef boost::shared_ptr<Filter> Ptr;
      
      virtual Error SetBandpassParameters(unsigned freq, unsigned lowCutoff, unsigned highCutoff) = 0;
    };
    
    /*
      FIR filter with fixed-point calculations
    */
    Error CreateFIRFilter(unsigned order, Filter::Ptr& filter);
  }
}

#endif //__FILTER_H_DEFINED__
