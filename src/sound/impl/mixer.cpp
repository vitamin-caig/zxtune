/*
Abstract:
  Mixer implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include "../mixer.h"
#include "../error_codes.h"

#include <tools.h>

#include <boost/mpl/if.hpp>
#include <boost/noncopyable.hpp>
#include <boost/integer/static_log2.hpp>

#include <algorithm>

#include <text/sound.h>

#define FILE_TAG 278565B1

namespace
{
  using namespace ZXTune::Sound;

  const std::size_t MAX_NATIVE_BITS = 8 * sizeof(unsigned);

  /*
  Simple mixer with fixed-point calculations
  */
  template<std::size_t InChannels>
  class Mixer : public ChainedReceiver, private boost::noncopyable
  {
    //determine type for intermediate value
    static const unsigned INTERMEDIATE_BITS_MIN =
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
    explicit Mixer(MixerData::Ptr data)
      : Data(data), Delegate()
    {
    }

    virtual void ApplySample(const Sample* input, unsigned channels)
    {
      if (Receiver::Ptr delegate = Delegate.lock())
      {
        if (channels != InChannels)
	{
	  throw Error(THIS_LINE, MIXER_CHANNELS_MISMATCH, TEXT_SOUND_ERROR_MIXER_MISMATCH);
	}
        const ChannelMixer* inChanMix(&Data->InMatrix[0]);
        const Sample preamp(Data->Preamp);

        BigSampleArray res = {0};
        std::size_t actChannels(0);
        for (const Sample* in = input; in != input + InChannels; ++in, ++inChanMix)
        {
          if (!inChanMix->Mute)
          {
            ++actChannels;
            const Sample* outChanMix(inChanMix->OutMatrix.Data);
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
      if (Receiver::Ptr delegate = Delegate.lock())
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
    Receiver::WeakPtr Delegate;
    SampleArray Result;
  };
}

namespace ZXTune
{
  namespace Sound
  {
    ChainedReceiver::Ptr CreateMixer(MixerData::Ptr data)
    {
      switch (const std::size_t size = data->InMatrix.size())
      {
      case 2:
        return ChainedReceiver::Ptr(new Mixer<2>(data));
      case 3:
        return ChainedReceiver::Ptr(new Mixer<3>(data));
      case 4:
        return ChainedReceiver::Ptr(new Mixer<4>(data));
      default:
        throw MakeFormattedError(THIS_LINE, MIXER_UNSUPPORTED, TEXT_SOUND_ERROR_MIXER_UNSUPPORTED, size);
      }
    }
  }
}
