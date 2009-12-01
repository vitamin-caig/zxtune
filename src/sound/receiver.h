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
    /// Template sound consuming interface
    template<class T>
    class BasicReceiver
    {
    public:
      typedef typename boost::shared_ptr<BasicReceiver<T> > Ptr;
      
      virtual ~BasicReceiver() {}
      
      virtual void ApplySample(const T& data) = 0;
      virtual void Flush() = 0;
    };
    
    template<class S, class T = S>
    class BasicConverter : public BasicReceiver<S>
    {
    public:
      typedef typename boost::shared_ptr<BasicConverter<S, T> > Ptr;
      
      virtual void SetEndpoint(typename BasicReceiver<T>::Ptr endpoint) = 0;
    };
    
    typedef BasicReceiver<MultiSample> Receiver;
    typedef BasicConverter<MultiSample> Converter;

    Receiver::Ptr CreateDummyReceiver();
  }
}

#endif //__SOUND_RECEIVER_H_DEFINED__
