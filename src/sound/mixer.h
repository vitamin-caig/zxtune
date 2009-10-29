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

#include "receiver.h"

namespace ZXTune
{
  namespace Sound
  {
    class Mixer : public MultichannelReceiver
    {
    public:
      typedef boost::shared_ptr<Mixer> Ptr;
      
      virtual void SetMatrix(const std::vector<MultiGain>& data) = 0;
      
      static Ptr Create(Receiver::Ptr receiver);
    };
  }
}

#endif //__SOUND_MIXER_H_DEFINED__
