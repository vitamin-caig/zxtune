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
  void FillMixer(const Parameters::Accessor& params, OneChannelMatrixMixer& mixer);
  void FillMixer(const Parameters::Accessor& params, TwoChannelsMatrixMixer& mixer);
  void FillMixer(const Parameters::Accessor& params, ThreeChannelsMatrixMixer& mixer);
  void FillMixer(const Parameters::Accessor& params, FourChannelsMatrixMixer& mixer);
}

#endif //SOUND_MIXER_FACTORY_H_DEFINED
