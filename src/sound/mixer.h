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
    template<unsigned Channels>
    class FixedChannelsMixer : public DataTransceiver<typename FixedChannelsSample<Channels>::Type, OutputSample> {};

    typedef FixedChannelsMixer<1> OneChannelMixer;
    typedef FixedChannelsMixer<2> TwoChannelsMixer;
    typedef FixedChannelsMixer<3> ThreeChannelsMixer;
    typedef FixedChannelsMixer<4> FourChannelsMixer;
  }
}

#endif //SOUND_MIXER_H_DEFINED
