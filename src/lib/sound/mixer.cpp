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
  class Mixer : public Convertor, private boost::noncopyable
  {
  public:
    Mixer(const ChannelMixer* matrix, Sample preamp)
      : Matrix(matrix), Preamp(preamp), Delegate()
    {

    }

    virtual void ApplySample(const Sample* input, std::size_t channels)
    {
      if (Receiver::Ptr delegate = Delegate)
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
              *out += *in * *outChanMix * Preamp;
            }
          }
        }
        std::transform(res, ArrayEnd(res), Result, std::bind2nd(std::divides<BigSample>(),
          BigSample(FIXED_POINT_PRECISION) * FIXED_POINT_PRECISION * actChannels));
        return delegate->ApplySample(Result, ArraySize(Result));
      }
    }

    virtual void Flush()
    {
      if (Receiver::Ptr delegate = Delegate)
      {
        return delegate->Flush();
      }
    }

    virtual void SetEndpoint(Receiver::Ptr delegate)
    {
      Delegate = delegate;
    }
  private:
    const ChannelMixer* const Matrix;
    const Sample Preamp;
    Receiver::Ptr Delegate;
    SampleArray Result;
  };
}

namespace ZXTune
{
  namespace Sound
  {
    Convertor::Ptr CreateMixer(const std::vector<ChannelMixer>& matrix, Sample preamp)
    {
      switch (matrix.size())
      {
      case 2:
        return Convertor::Ptr(new Mixer<2>(&matrix[0], preamp));
      case 3:
        return Convertor::Ptr(new Mixer<3>(&matrix[0], preamp));
      case 4:
        return Convertor::Ptr(new Mixer<4>(&matrix[0], preamp));
      default:
        assert(!"Invalid channels number specified");
        return Convertor::Ptr();
      }
    }
  }
}
