/*
Abstract:
  Some helper functions implementation for sound interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include "../receiver.h"

namespace
{
  using namespace ZXTune::Sound;
  
  class DummyReceiver : public Receiver
  {
  public:
    DummyReceiver()
    {
    }
    
    virtual void ApplySample(const MultiSample& /*param*/)
    {
    }
    
    virtual void Flush()
    {
    }
  };
}

namespace ZXTune
{
  namespace Sound
  {
    Receiver::Ptr CreateDummyReceiver()
    {
      return Receiver::Ptr(new DummyReceiver());
    }
  }
}
