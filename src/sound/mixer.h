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

#include <error.h>
#include <sound/receiver.h>

namespace ZXTune
{
  namespace Sound
  {
    class Mixer : public BasicConverter<std::vector<Sample>, MultiSample>
    {
    public:
      typedef boost::shared_ptr<Mixer> Ptr;
      
      virtual Error SetMatrix(const std::vector<MultiGain>& data) = 0;
    };
    
    Error CreateMixer(unsigned channels, Mixer::Ptr& result);
  }
}

#endif //__SOUND_MIXER_H_DEFINED__
