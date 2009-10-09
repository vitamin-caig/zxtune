/*
Abstract:
  Defenition of sound receiver interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef __SOUND_RECEIVER_H_DEFINED__
#define __SOUND_RECEIVER_H_DEFINED__

#include "sound_types.h"

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

namespace ZXTune
{
  namespace Sound
  {
    /// Main sound consuming interface
    class Receiver
    {
    public:
      typedef boost::shared_ptr<Receiver> Ptr;
      typedef boost::weak_ptr<Receiver> WeakPtr;
      
      virtual ~Receiver() {}
      
      virtual void ApplySample(const Sample* data, unsigned channels) = 0;
      virtual void Flush() = 0;
    };
    
    /// Supporting for chain receivers
    class ChainedReceiver : public Receiver
    {
    public:
      typedef boost::shared_ptr<ChainedReceiver> Ptr;
      
      virtual void SetEndpoint(Receiver::Ptr endpoint) = 0;
    };
  }
}

#endif //__SOUND_RECEIVER_H_DEFINED__
