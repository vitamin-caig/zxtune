#include "mixer.h"

#include <tools.h>

#include <algorithm>

namespace
{
  using namespace ZXTune::Sound;

  /*
  Simple mixer with fixed-point calculations
  */
  template<std::size_t InChannels>
  class Mixer : public Receiver
  {
  public:
    Mixer(const SampleArray* matrix, Receiver* delegate)
      : Matrix(matrix), Delegate(delegate)
    {

    }

    virtual void ApplySample(const Sample* input, std::size_t channels)
    {
      assert(channels == InChannels || !"Invalid input channels mixer specified");
      const SampleArray* inChanMix(Matrix);

      BigSampleArray res = {0};
      for (const Sample* in = input; in != input + InChannels; ++in, ++inChanMix)
      {
        const Sample* outChanMix(*inChanMix);
        for (BigSample* out = res; out != ArrayEnd(res); ++out, ++outChanMix)
        {
          *out += *in * *outChanMix;
        }
      }
      std::transform(res, ArrayEnd(res), Result, std::bind2nd(std::divides<BigSample>(), FIXED_POINT_PRECISION * InChannels));
      return Delegate->ApplySample(Result, ArraySize(Result));
    }
  private:
    const SampleArray* const Matrix;
    Receiver* const Delegate;
    SampleArray Result;
  };
}

namespace ZXTune
{
  namespace Sound
  {
    Receiver::Ptr CreateMixer(std::size_t inChannels, const SampleArray* matrix, Receiver* receiver)
    {
      switch (inChannels)
      {
      case 2:
        return Receiver::Ptr(new Mixer<2>(matrix, receiver));
      case 3:
        return Receiver::Ptr(new Mixer<3>(matrix, receiver));
      case 4:
        return Receiver::Ptr(new Mixer<4>(matrix, receiver));
      default:
        assert(!"Invalid channels number specified");
        return Receiver::Ptr(0);
      }
    }
  }
}
