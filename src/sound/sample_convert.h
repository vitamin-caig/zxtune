/**
*
* @file      sound/sample_convert.h
* @brief     Sample conversion functions
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef SOUND_SAMPLE_CONVERT_H_DEFINED
#define SOUND_SAMPLE_CONVERT_H_DEFINED

//local includes
#include "sound_types.h"

namespace ZXTune
{
  namespace Sound
  {
    template<class T>
    inline Sample ToSample(T val);

    template<>
    inline Sample ToSample(Sample val)
    {
      return val;
    }
  }
}

#endif //SOUND_SAMPLE_CONVERT_H_DEFINED
