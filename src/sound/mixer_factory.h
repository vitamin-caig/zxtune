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
    OneChannelMixer::Ptr CreateOneChannelMixer(Parameters::Accessor::Ptr params);
    TwoChannelsMixer::Ptr CreateTwoChannelsMixer(Parameters::Accessor::Ptr params);
    ThreeChannelsMixer::Ptr CreateThreeChannelsMixer(Parameters::Accessor::Ptr params);
    FourChannelsMixer::Ptr CreateFourChannelsMixer(Parameters::Accessor::Ptr params);

    template<unsigned Channels>
    typename FixedChannelsMixer<Channels>::Ptr CreateMixer(Parameters::Accessor::Ptr params);

    MultichannelMixer::Ptr CreateMultichannelMixer(uint_t channels, Parameters::Accessor::Ptr params);
  }
}

#endif //SOUND_MIXER_FACTORY_H_DEFINED
