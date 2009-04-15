#ifndef __RENDERER_H__
#define __RENDERER_H__

#include <sound.h>

namespace ZXTune
{
  namespace Sound
  {
    /*
      Renderers. Data for single sample stored sequently.
      size in bytes = size in samples * Channels * sizeof(Sample)
    */
    template<std::size_t Channels>
    class CallbackRenderer : public Receiver
    {
    public:
      //size in bytes
      void (*ReadyCallback)(const void*, std::size_t, void*);

      //size in multisamples
      CallbackRenderer(std::size_t size, ReadyCallback* callback, void* userData)
        : Buffer(size), BufferEnd(&Buffer[size]), Callback(callback), UserData(userData), Position(&Buffer[0])
      {
        assert(Callback || !"No callback specified");
      }

      virtual void ApplySample(const Sample* input, std::size_t channels)
      {
        assert(channels == Channels || !"Invalid input channels CallbackRenderer specified");
        std::memcpy(Position, input, sizeof(*Position));
        if (BufferEnd == ++Position)
        {
          Position = &Buffer[0];
          return (*Callback)((*Position)[0], Buffer.size() * sizeof(MultiSample), UserData);
        }
      }
    private:
      typedef Sample[Channels] MultiSample;
      std::vector<MultiSample> Buffer;
      const MultiSample* const BufferEnd;
      ReadyCallback Callback;
      void* const UserData;
      MultiSample* Position;
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
    inline Receiver::Ptr CreateRenderer(std::size_t size, CallbackRenderer<Channels>::ReadyCallback callback, void* data = 0)
    {
      return Receiver::Ptr(new CallbackRenderer<Channels>(size, callback, data));
    }

    template<std::size_t Channels, bool Cyclic = true>
    inline Receiver::Ptr CreateRenderer(Sample* buffer, std::size_t size)
    {
      return Receiver::Ptr(new CallbackRenderer<Channels, Cyclic>(buffer, size));
    }
  }
}

#endif //__RENDERER_H__
