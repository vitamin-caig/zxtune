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

#include "internal_types.h"

#include <tools.h>

#include <boost/bind.hpp>
#include <boost/mpl/if.hpp>
#include <boost/noncopyable.hpp>
#include <boost/integer/static_log2.hpp>

#include <algorithm>

#include <text/sound.h>

#define FILE_TAG 278565B1

namespace
{
  using namespace ZXTune::Sound;

  typedef unsigned NativeType;
  typedef uint64_t MaxBigType;

  const MaxBigType MAX_MIXER_CHANNELS = MaxBigType(1) << (8 * sizeof(MaxBigType) - 8 * sizeof(Sample) - 
    boost::static_log2<FIXED_POINT_PRECISION>::value);
  
  inline bool FindOverloadedGain(const MultiGain& mg)
  {
    return mg.end() != std::find_if(mg.begin(), mg.end(), !boost::bind(in_range<Gain>, _1, 0.0f, 1.0f));
  }
    
  template<unsigned InChannels>
  class FastMixer : public Mixer, private boost::noncopyable
  {
    //determine type for intermediate value
    static const unsigned INTERMEDIATE_BITS_MIN =
      8 * sizeof(Sample) +                               //input sample
      boost::static_log2<FIXED_POINT_PRECISION>::value + //mixer
      boost::static_log2<InChannels>::value;             //channels count

    // calculate most suitable type for intermediate value storage
    typedef typename boost::mpl::if_c<
      INTERMEDIATE_BITS_MIN <= 8 * sizeof(NativeType),
      NativeType,
      MaxBigType
    >::type BigSample;
    typedef boost::array<BigSample, OUTPUT_CHANNELS> MultiBigSample;
    
    typedef boost::array<NativeType, OUTPUT_CHANNELS> MultiFixed;
  public:
    FastMixer()
      : Endpoint(CreateDummyReceiver())
    {
    }

    virtual void ApplySample(const std::vector<Sample>& inData)
    {
      if (inData.size() != InChannels)
      {
        assert(!"Mixer::ApplySample channels mismatch");
        return;//do not do anything
      }
      // pass along input channels due to input data structure
      const Sample* const input(&inData[0]);
      MultiBigSample res = { {0} };
      for (unsigned inChan = 0; inChan != InChannels; ++inChan)
      {
        const Sample in(input[inChan]);
        const MultiFixed& inChanMix(Matrix[inChan]);
        for (unsigned outChan = 0; outChan != OUTPUT_CHANNELS; ++outChan)
        {
          res[outChan] += inChanMix[outChan] * in;
        }
      }
      MultiSample result;
      std::transform(res.begin(), res.end(), result.begin(), std::bind2nd(std::divides<BigSample>(),
        BigSample(FIXED_POINT_PRECISION) * InChannels));
      return Endpoint->ApplySample(result);
    }
    
    virtual void Flush()
    {
      Endpoint->Flush();
    }
    
    virtual void SetEndpoint(Receiver::Ptr rcv)
    {
      Endpoint = rcv ? rcv : CreateDummyReceiver();
    }
    
    virtual Error SetMatrix(const std::vector<MultiGain>& data)
    {
      if (data.size() != InChannels)
      {
        return Error(THIS_LINE, MIXER_INVALID_MATRIX, TEXT_SOUND_ERROR_MIXER_INVALID_MATRIX_CHANNELS);
      }
      const std::vector<MultiGain>::const_iterator it(std::find_if(data.begin(), data.end(),
        FindOverloadedGain));
      if (it != data.end())
      {
        return Error(THIS_LINE, MIXER_INVALID_MATRIX, TEXT_SOUND_ERROR_MIXER_INVALID_MATRIX_GAIN);
      }
      std::transform(data.begin(), data.end(), Matrix.begin(), MultiGain2MultiFixed<NativeType>);
      return Error();
    }
    
  private:
    Receiver::Ptr Endpoint;
    boost::array<MultiFixed, InChannels> Matrix;
  };
}

namespace ZXTune
{
  namespace Sound
  {
    Error CreateMixer(unsigned channels, Mixer::Ptr& ptr)
    {
      switch (channels)
      {
      case 1:
        ptr.reset(new FastMixer<1>());
        break;
      case 2:
        ptr.reset(new FastMixer<2>());
        break;
      case 3:
        ptr.reset(new FastMixer<3>());
        break;
      case 4:
        ptr.reset(new FastMixer<4>());
        break;
      default:
        assert(!"Mixer: invalid channels count specified");
        return MakeFormattedError(THIS_LINE, MIXER_UNSUPPORTED, TEXT_SOUND_ERROR_MIXER_UNSUPPORTED, channels);
      }
      return Error();
    }
  }
}
