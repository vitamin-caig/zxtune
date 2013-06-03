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
  OneChannelStreamMixer::Ptr CreateOneChannelStreamMixer(Parameters::Accessor::Ptr params);
  TwoChannelsStreamMixer::Ptr CreateTwoChannelsStreamMixer(Parameters::Accessor::Ptr params);
  ThreeChannelsStreamMixer::Ptr CreateThreeChannelsStreamMixer(Parameters::Accessor::Ptr params);
  FourChannelsStreamMixer::Ptr CreateFourChannelsStreamMixer(Parameters::Accessor::Ptr params);

  //tag is used to make set of functions, else they will differs only by return type that is prohibited
  inline OneChannelStreamMixer::Ptr CreateStreamMixer(Parameters::Accessor::Ptr params, const MultichannelSample<1>& /*tag*/)
  {
    return CreateOneChannelStreamMixer(params);
  }

  inline TwoChannelsStreamMixer::Ptr CreateStreamMixer(Parameters::Accessor::Ptr params, const MultichannelSample<2>& /*tag*/)
  {
    return CreateTwoChannelsStreamMixer(params);
  }

  inline ThreeChannelsStreamMixer::Ptr CreateStreamMixer(Parameters::Accessor::Ptr params, const MultichannelSample<3>& /*tag*/)
  {
    return CreateThreeChannelsStreamMixer(params);
  }

  inline FourChannelsStreamMixer::Ptr CreateStreamMixer(Parameters::Accessor::Ptr params, const MultichannelSample<4>& /*tag*/)
  {
    return CreateFourChannelsStreamMixer(params);
  }

  OneChannelMixer::Ptr CreateOneChannelMixer(Parameters::Accessor::Ptr params);
  TwoChannelsMixer::Ptr CreateTwoChannelsMixer(Parameters::Accessor::Ptr params);
  ThreeChannelsMixer::Ptr CreateThreeChannelsMixer(Parameters::Accessor::Ptr params);
  FourChannelsMixer::Ptr CreateFourChannelsMixer(Parameters::Accessor::Ptr params);

  //tag is used to make set of functions, else they will differs only by return type that is prohibited
  inline OneChannelMixer::Ptr CreateMixer(Parameters::Accessor::Ptr params, const MultichannelSample<1>& /*tag*/)
  {
    return CreateOneChannelMixer(params);
  }

  inline TwoChannelsMixer::Ptr CreateMixer(Parameters::Accessor::Ptr params, const MultichannelSample<2>& /*tag*/)
  {
    return CreateTwoChannelsMixer(params);
  }

  inline ThreeChannelsMixer::Ptr CreateMixer(Parameters::Accessor::Ptr params, const MultichannelSample<3>& /*tag*/)
  {
    return CreateThreeChannelsMixer(params);
  }

  inline FourChannelsMixer::Ptr CreateMixer(Parameters::Accessor::Ptr params, const MultichannelSample<4>& /*tag*/)
  {
    return CreateFourChannelsMixer(params);
  }

  FixedChannelsMatrixMixer<3>::Matrix ReadThreeChannelsMixerMatrix(const Parameters::Accessor& params);
}

#endif //SOUND_MIXER_FACTORY_H_DEFINED
