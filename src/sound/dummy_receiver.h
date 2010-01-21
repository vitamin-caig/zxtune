/*
Abstract:
  Dummy sound receiver helper

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef __SOUND_DUMMY_RECEIVER_H_DEFINED__
#define __SOUND_DUMMY_RECEIVER_H_DEFINED__

#include <sound/receiver.h>

namespace ZXTune
{
  namespace Sound
  {
    template<class T>
    class DummyReceiverObject : public T
    {
    public:
      DummyReceiverObject()
      {
      }

      virtual void ApplySample(const typename T::DataType& /*data*/)
      {
      }

      virtual void Flush()
      {
      }
    };

    inline Receiver::Ptr CreateDummyReceiver()
    {
      return Receiver::Ptr(new DummyReceiverObject<Receiver>());
    }
  }
}

#endif //__SOUND_DUMMY_RECEIVER_H_DEFINED__
