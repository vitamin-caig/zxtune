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

#include <vector>

namespace ZXTune
{
  namespace Sound
  {
    /// Multichannel sound consuming interface
    class MultichannelReceiver
    {
    public:
      typedef boost::shared_ptr<MultichannelReceiver> Ptr;
      
      virtual ~MultichannelReceiver() {}
      
      virtual void ApplySample(const std::vector<Sample>& data) = 0;
      virtual void Flush() = 0;
    };
    
    /// Sound consuming interface
    class SoundReceiver
    {
    public:
      typedef boost::shared_ptr<SoundReceiver> Ptr;
      
      virtual ~SoundReceiver() {}
      
      virtual void ApplySample(const MultiSample& data) = 0;
      virtual void Flush() = 0;
    };
    
    /// Sound converting interface
    class SoundConverter : public SoundReceiver
    {
    public:
      typedef boost::shared_ptr<SoundConverter> Ptr;
      
      virtual void SetEndpoint(SoundReceiver::Ptr endpoint) = 0;
    };
  }
}

#endif //__SOUND_RECEIVER_H_DEFINED__
