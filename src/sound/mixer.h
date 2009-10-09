/*
Abstract:
  Defenition of mixing-related functionality

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef __SOUND_MIXER_H_DEFINED__
#define __SOUND_MIXER_H_DEFINED__

#include "sound_types.h"
#include "receiver.h"

namespace ZXTune
{
  namespace Sound
  {
    /// Mixing point for one input channel to all outputs
    struct ChannelMixer
    {
      ChannelMixer() : Mute(false)
      {
      }
      explicit ChannelMixer(const Multisample& ms) : Mute(false), OutMatrix(ms)
      {
      }
      bool Mute;
      Multisample OutMatrix;
    };

    struct MixerData
    {
      typedef boost::shared_ptr<MixerData> Ptr;

      MixerData() : InMatrix(), Preamp()
      {
      }
      std::vector<ChannelMixer> InMatrix;
      Sample Preamp;
    };

    ChainedReceiver::Ptr CreateMixer(MixerData::Ptr data);
  }
}

#endif //__SOUND_MIXER_H_DEFINED__
