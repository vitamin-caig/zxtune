#ifndef __RENDERER_H__
#define __RENDERER_H__

#include <sound.h>

#include <cassert>

namespace ZXTune
{
  namespace Sound
  {
    /*
      Renderers. Data for single sample stored sequently.
      size in bytes = size in samples * Channels * sizeof(Sample)
    */
    typedef void (*ReadyCallback)(const void*, std::size_t, void*);

    template<std::size_t Channels>
    class CallbackRenderer : public Receiver
    {
    public:
      //size in bytes

      //size in multisamples
      CallbackRenderer(std::size_t size, ReadyCallback callback, void* userData)
        : Buffer(new MultiSample[size]), BufferEnd(Buffer + size), Callback(callback)
        , UserData(userData), Position(Buffer)
      {
        assert(Callback || !"No callback specified");
      }

      virtual ~CallbackRenderer()
      {
        delete[] Buffer;
      }

      virtual void ApplySample(const Sample* input, std::size_t channels)
      {
        assert(channels == Channels || !"Invalid input channels CallbackRenderer specified");
        std::memcpy(Position, input, sizeof(typename MultiSample));
        if (BufferEnd == ++Position)
        {
          Position = Buffer;
          return (*Callback)(Buffer, (BufferEnd - Buffer) * sizeof(MultiSample), UserData);
        }
      }
    private:
      typedef Sample MultiSample[Channels];
      MultiSample* Buffer;
      const MultiSample* const BufferEnd;
      MultiSample* Position;
      ReadyCallback Callback;
      void* const UserData;
    };

    template<std::size_t Channels, bool Cyclic>
    class BufferRenderer : public Receiver
    {
    public:
      //size in bytes
      BufferRenderer(void* buffer, std::size_t size)
        : Buffer(buffer), BufferEnd(buffer + size / Channels * sizeof(*Buffer)), Position(Buffer)
      {
        assert(0 == size % (Channels * sizeof(*Buffer)) || !"Invalid buffer size specified");
      }

      virtual void ApplySample(const Sample* input, std::size_t channels)
      {
        assert(channels == Channels || !"Invalid input channels BufferRenderer specified");
        if (BufferEnd != Position)
        {
          std::memcpy(Position, input, Channels);
          Position += Channels;
          if (BufferEnd == Position && Cyclic)
          {
            Position = Buffer;
          }
        }
      }
    private:
      const Sample* const Buffer;
      const Sample* const BufferEnd;
      Sample* Position;
    };

    template<std::size_t Channels>
    inline Receiver::Ptr CreateRenderer(std::size_t size, ReadyCallback callback, void* data = 0)
    {
      return Receiver::Ptr(new CallbackRenderer<Channels>(size, callback, data));
    }

    template<std::size_t Channels>
    inline Receiver::Ptr CreateRenderer(Sample* buffer, std::size_t size)
    {
      return Receiver::Ptr(new CallbackRenderer<Channels, true>(buffer, size));
    }
  }
}

#endif //__RENDERER_H__
