#include "mixer.h"

#include <tools.h>

#include <boost/mpl/if.hpp>
#include <boost/noncopyable.hpp>
#include <boost/integer/static_log2.hpp>

#include <algorithm>

namespace
{
  using namespace ZXTune::Sound;

  const std::size_t MAX_NATIVE_BITS = 8 * sizeof(unsigned);

  /*
  Simple mixer with fixed-point calculations
  */
  template<std::size_t InChannels>
  class Mixer : public Convertor, private boost::noncopyable
  {
    //determine type for intermediate value
    static const std::size_t INTERMEDIATE_BITS_MIN =
      8 * sizeof(Sample) +                               //input sample
      boost::static_log2<FIXED_POINT_PRECISION>::value + //mixer
      boost::static_log2<FIXED_POINT_PRECISION>::value + //preamp
      boost::static_log2<InChannels>::value;             //channels count

    // calculate most suitable type for intermediate value storage
    typedef typename boost::mpl::if_c<
      INTERMEDIATE_BITS_MIN <= MAX_NATIVE_BITS,
      unsigned,
      uint64_t
    >::type BigSample;
    typedef BigSample BigSampleArray[OUTPUT_CHANNELS];
  public:
    Mixer(MixerData::Ptr data)
      : Data(data), Delegate()
    {
    }

    virtual void ApplySample(const Sample* input, std::size_t channels)
    {
      if (Receiver::Ptr delegate = Delegate)
      {
        assert(channels == InChannels || !"Invalid input channels mixer specified");
        const ChannelMixer* inChanMix(&Data->InMatrix[0]);
        const Sample preamp(Data->Preamp);

        BigSampleArray res = {0};
        std::size_t actChannels(0);
        for (const Sample* in = input; in != input + InChannels; ++in, ++inChanMix)
        {
          if (!inChanMix->Mute)
          {
            ++actChannels;
            const Sample* outChanMix(inChanMix->OutMatrix);
            for (BigSample* out = res; out != ArrayEnd(res); ++out, ++outChanMix)
            {
              *out += *in * *outChanMix * preamp;
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
    MixerData::Ptr Data;
    Receiver::Ptr Delegate;
    SampleArray Result;
  };
}

namespace ZXTune
{
  namespace Sound
  {
    Convertor::Ptr CreateMixer(MixerData::Ptr data)
    {
      switch (data->InMatrix.size())
      {
      case 2:
        return Convertor::Ptr(new Mixer<2>(data));
      case 3:
        return Convertor::Ptr(new Mixer<3>(data));
      case 4:
        return Convertor::Ptr(new Mixer<4>(data));
      default:
        assert(!"Invalid channels number specified");
        return Convertor::Ptr();
      }
    }
  }
}
