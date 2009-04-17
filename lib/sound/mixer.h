#ifndef __MIXER_H_DEFINED__
#define __MIXER_H_DEFINED__

#include <sound.h>

#include <cassert>

namespace ZXTune
{
  namespace Sound
  {
    Receiver::Ptr CreateMixer(std::size_t inChannels, const SampleArray* matrix, Receiver* receiver);
  }
}

#endif //__MIXER_H_DEFINED__
