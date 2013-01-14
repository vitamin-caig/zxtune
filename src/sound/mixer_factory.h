/**
*
* @file      sound/mixer_factory.h
* @brief     Factory to create mixer
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef SOUND_MIXER_FACTORY_H_DEFINED
#define SOUND_MIXER_FACTORY_H_DEFINED

//common includes
#include <parameters.h>
//library includes
#include <sound/mixer.h>

namespace ZXTune
{
  namespace Sound
  {
    Mixer::Ptr CreateMixer(uint_t channels, Parameters::Accessor::Ptr params);
    Mixer::Ptr CreatePollingMixer(uint_t channels, Parameters::Accessor::Ptr params);
  }
}

#endif //SOUND_MIXER_FACTORY_H_DEFINED
