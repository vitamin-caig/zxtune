#include "renderer.h"

#include <tools.h>

#include <boost/static_assert.hpp>

namespace
{
  using namespace ZXTune::Sound;

  struct SampleHelper
  {
    SampleArray Array;
  };

  BOOST_STATIC_ASSERT(sizeof(SampleHelper) == sizeof(SampleArray));

  class CallbackRenderer : public Receiver
  {
  public:
    //size in multisamples
    CallbackRenderer(std::size_t size, ReadyCallback callback, void* userData)
      : Buffer(size), BufferEnd(&Buffer[size]), Position(&Buffer[0])
      , Callback(callback), UserData(userData)
    {
      assert(Callback || !"No callback specified");
    }

    virtual void ApplySample(const Sample* input, std::size_t channels)
    {
      assert(channels == ArraySize(Position->Array) || !"Invalid input channels CallbackRenderer specified");
      std::memcpy(Position->Array, input, sizeof(Position->Array));
      if (BufferEnd == ++Position)
      {
        Position = &Buffer[0];
        return (*Callback)(Position, (BufferEnd - Position) * sizeof(Position->Array), UserData);
      }
    }

    virtual void Flush()
    {
      const SampleHelper* const endOf(Position);
      Position = &Buffer[0];
      return (*Callback)(Position, (endOf - Position) * sizeof(Position->Array), UserData);
    }

  private:
    std::vector<SampleHelper> Buffer;
    const SampleHelper* const BufferEnd;
    SampleHelper* Position;
    ReadyCallback Callback;
    void* const UserData;
  };
/*
  class BufferRenderer : public Receiver
  {
  public:
    //size in bytes
    BufferRenderer(void* buffer, std::size_t size, bool cyclic = true)
      : Buffer(static_cast<SampleHelper*>(buffer)), BufferEnd(Buffer + size / sizeof(Buffer->Array))
      , Position(Buffer), Cyclic(cyclic)
    {
      assert(0 == size % (sizeof(Buffer->Array)) || !"Invalid buffer size specified");
    }

    virtual void ApplySample(const Sample* input, std::size_t channels)
    {
      assert(channels == ArraySize(Buffer->Array) || !"Invalid input channels BufferRenderer specified");
      if (BufferEnd != Position)
      {
        std::memcpy(Position->Array, input, sizeof(Buffer->Array));
        if (BufferEnd == ++Position && Cyclic)
        {
          Position = Buffer;
        }
      }
    }

    virtual void Flush()
    {
      Position
    }
  private:
    SampleHelper* const Buffer;
    SampleHelper* const BufferEnd;
    SampleHelper* Position;
    const bool Cyclic;
  };
  */
}

namespace ZXTune
{
  namespace Sound
  {
    Receiver::Ptr CreateCallbackRenderer(std::size_t size, ReadyCallback callback, void* data)
    {
      return Receiver::Ptr(new CallbackRenderer(size, callback, data));
    }
/*
    Receiver::Ptr CreateBufferRenderer(Sample* buffer, std::size_t size)
    {
      return Receiver::Ptr(new BufferRenderer(buffer, size));
    }
*/
  }
}