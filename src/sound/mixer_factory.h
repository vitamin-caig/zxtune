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

    //tag is used to make set of functions, else they will differs only by return type that is prohibited
    OneChannelMixer::Ptr CreateMixer(Parameters::Accessor::Ptr params, const FixedChannelsSample<1>& /*tag*/)
    {
      return CreateOneChannelMixer(params);
    }

    TwoChannelsMixer::Ptr CreateMixer(Parameters::Accessor::Ptr params, const FixedChannelsSample<2>& /*tag*/)
    {
      return CreateTwoChannelsMixer(params);
    }

    ThreeChannelsMixer::Ptr CreateMixer(Parameters::Accessor::Ptr params, const FixedChannelsSample<3>& /*tag*/)
    {
      return CreateThreeChannelsMixer(params);
    }

    FourChannelsMixer::Ptr CreateMixer(Parameters::Accessor::Ptr params, const FixedChannelsSample<4>& /*tag*/)
    {
      return CreateFourChannelsMixer(params);
    }
  }
}

#endif //SOUND_MIXER_FACTORY_H_DEFINED
