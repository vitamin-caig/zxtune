#include "mixer.h"

#include <tools.h>

#include <boost/noncopyable.hpp>

#include <algorithm>

namespace
{
  using namespace ZXTune::Sound;

  /*
  Simple mixer with fixed-point calculations
  */
  template<std::size_t InChannels>
  class Mixer : public Receiver, private boost::noncopyable
  {
  public:
    Mixer(const ChannelMixer* matrix, Receiver& delegate)
      : Matrix(matrix), Delegate(delegate)
    {

    }

    virtual void ApplySample(const Sample* input, std::size_t channels)
    {
      assert(channels == InChannels || !"Invalid input channels mixer specified");
      const ChannelMixer* inChanMix(Matrix);

      BigSampleArray res = {0};
      std::size_t actChannels(0);
      for (const Sample* in = input; in != input + InChannels; ++in, ++inChanMix)
      {
        if (!inChanMix->Mute)
        {
          ++actChannels;
          const Sample* outChanMix(inChanMix->Matrix);
          for (BigSample* out = res; out != ArrayEnd(res); ++out, ++outChanMix)
          {
            *out += *in * *outChanMix;
          }
        }
      }
      std::transform(res, ArrayEnd(res), Result, std::bind2nd(std::divides<BigSample>(), BigSample(FIXED_POINT_PRECISION) * actChannels));
      return Delegate.ApplySample(Result, ArraySize(Result));
    }

    virtual void Flush()
    {
      return Delegate.Flush();
    }

  private:
    const ChannelMixer* const Matrix;
    Receiver& Delegate;
    SampleArray Result;
  };
}

namespace ZXTune
{
  namespace Sound
  {
    Receiver::Ptr CreateMixer(const std::vector<ChannelMixer>& matrix, Receiver& receiver)
    {
      switch (matrix.size())
      {
      case 2:
        return Receiver::Ptr(new Mixer<2>(&matrix[0], receiver));
      case 3:
        return Receiver::Ptr(new Mixer<3>(&matrix[0], receiver));
      case 4:
        return Receiver::Ptr(new Mixer<4>(&matrix[0], receiver));
      default:
        assert(!"Invalid channels number specified");
        return Receiver::Ptr(0);
      }
    }
  }
}
