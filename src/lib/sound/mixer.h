#ifndef __MIXER_H_DEFINED__
#define __MIXER_H_DEFINED__

#include <sound.h>

#include <cassert>

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
      explicit ChannelMixer(const SampleArray& ar) : Mute(false)
      {
        std::memcpy(OutMatrix, &ar[0], sizeof(OutMatrix));
      }
      bool Mute;
      SampleArray OutMatrix;
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

    Convertor::Ptr CreateMixer(MixerData::Ptr data);
  }
}

#endif //__MIXER_H_DEFINED__
