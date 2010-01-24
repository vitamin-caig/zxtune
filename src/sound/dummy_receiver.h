/**
*
* @file      sound/dummy_receiver.h
* @brief     Dummy sound receiver helper
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#ifndef __SOUND_DUMMY_RECEIVER_H_DEFINED__
#define __SOUND_DUMMY_RECEIVER_H_DEFINED__

#include <sound/receiver.h>

namespace ZXTune
{
  namespace Sound
  {
    //! @brief Simple dummy template implementation
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

    //! @brief Creating instance of dummy receiver
    inline Receiver::Ptr CreateDummyReceiver()
    {
      return Receiver::Ptr(new DummyReceiverObject<Receiver>());
    }
  }
}

#endif //__SOUND_DUMMY_RECEIVER_H_DEFINED__
