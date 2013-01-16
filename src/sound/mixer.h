/**
*
* @file      sound/mixer.h
* @brief     Defenition of mixing-related functionality
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef SOUND_MIXER_H_DEFINED
#define SOUND_MIXER_H_DEFINED

//library includes
#include <sound/receiver.h>

namespace ZXTune
{
  namespace Sound
  {
    //! @brief Abstract mixer interface
    typedef DataTransceiver<MultichannelSample, MultiSample> Mixer;
  }
}

#endif //SOUND_MIXER_H_DEFINED
