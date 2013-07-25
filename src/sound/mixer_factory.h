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
#include <sound/matrix_mixer.h>

namespace Sound
{
  void FillMixer(const Parameters::Accessor& params, ThreeChannelsMatrixMixer& mixer);
  void FillMixer(const Parameters::Accessor& params, FourChannelsMatrixMixer& mixer);

  Parameters::Accessor::Ptr CreateMixerNotificationParameters(Parameters::Accessor::Ptr delegate, ThreeChannelsMatrixMixer::Ptr mixer);
  Parameters::Accessor::Ptr CreateMixerNotificationParameters(Parameters::Accessor::Ptr delegate, FourChannelsMatrixMixer::Ptr mixer);
}

#endif //SOUND_MIXER_FACTORY_H_DEFINED
