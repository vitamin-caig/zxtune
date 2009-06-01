#ifndef __MIXER_H_DEFINED__
#define __MIXER_H_DEFINED__

#include <sound.h>

#include <cassert>

namespace ZXTune
{
  namespace Sound
  {
    Convertor::Ptr CreateMixer(const std::vector<ChannelMixer>& matrix, Sample preamp);
  }
}

#endif //__MIXER_H_DEFINED__
