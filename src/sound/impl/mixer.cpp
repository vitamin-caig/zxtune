/*
Abstract:
  Mixer implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/
#include "internal_types.h"

#include <tools.h>
#include <error_tools.h>
#include <sound/dummy_receiver.h>
#include <sound/error_codes.h>
#include <sound/mixer.h>

#include <algorithm>
#include <numeric>

#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include <boost/integer/static_log2.hpp>
#include <boost/mpl/if.hpp>

#include <text/sound.h>

#define FILE_TAG 278565B1

namespace
{
  using namespace ZXTune::Sound;

  //using unsigned as a native type gives better performance (very strange...), at least at gcc
  typedef unsigned NativeType;
  typedef uint64_t MaxBigType;

  const MaxBigType MAX_MIXER_CHANNELS = MaxBigType(1) << (8 * sizeof(MaxBigType) - 8 * sizeof(Sample) -
    boost::static_log2<FIXED_POINT_PRECISION>::value);
  
  inline bool FindOverloadedGain(const MultiGain& mg)
  {
    return mg.end() != std::find_if(mg.begin(), mg.end(), !boost::bind(in_range<Gain>, _1, 0.0f, 1.0f));
  }
   
  template<uint_t InChannels>
  class FastMixer : public Mixer, private boost::noncopyable
  {
    //determine type for intermediate value
    static const uint_t INTERMEDIATE_BITS_MIN =
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

    //to prevent zero divider
    static inline BigSample FixDivider(BigSample divider)
    {
      return divider ? divider : 1;
    }
    
    //divider+matrix
    static inline BigSample AddDivider(BigSample divider, NativeType matrix)
    {
      return divider + (matrix ? FIXED_POINT_PRECISION : 0);
    }
    
    //divider+matrix
    static inline MultiBigSample AddBigsamples(const MultiBigSample& lh, const MultiFixed& rh)
    {
      MultiBigSample res;
      std::transform(lh.begin(), lh.end(), rh.begin(), res.begin(), AddDivider);
      return res;
    }
  public:
    FastMixer()
      : Endpoint(CreateDummyReceiver())
    {
      std::fill(Matrix.front().begin(), Matrix.front().end(), FIXED_POINT_PRECISION);
      std::fill(Matrix.begin(), Matrix.end(), Matrix.front());
      std::fill(Dividers.begin(), Dividers.end(), FIXED_POINT_PRECISION * InChannels);
    }

    virtual void ApplySample(const std::vector<Sample>& inData)
    {
      if (inData.size() != InChannels)
      {
        assert(!"Mixer::ApplySample channels mismatch");
        return;//do not do anything
      }
      // pass along input channels due to input data structure
      MultiBigSample res = { {0} };
      for (uint_t inChan = 0; inChan != InChannels; ++inChan)
      {
        const NativeType in(inData[inChan]);
        const MultiFixed& inChanMix(Matrix[inChan]);
        for (uint_t outChan = 0; outChan != OUTPUT_CHANNELS; ++outChan)
        {
          res[outChan] += inChanMix[outChan] * in;
        }
      }
      MultiSample result;
      std::transform(res.begin(), res.end(), Dividers.begin(), result.begin(), std::divides<BigSample>());
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
        return Error(THIS_LINE, MIXER_INVALID_PARAMETER, TEXT_SOUND_ERROR_MIXER_INVALID_MATRIX_CHANNELS);
      }
      const std::vector<MultiGain>::const_iterator it(std::find_if(data.begin(), data.end(),
        FindOverloadedGain));
      if (it != data.end())
      {
        return Error(THIS_LINE, MIXER_INVALID_PARAMETER, TEXT_SOUND_ERROR_MIXER_INVALID_MATRIX_GAIN);
      }
      std::transform(data.begin(), data.end(), Matrix.begin(), MultiGain2MultiFixed<NativeType>);
      Dividers = std::accumulate(Matrix.begin(), Matrix.end(), MultiBigSample(), AddBigsamples);
      // prevent empty dividers
      std::transform(Dividers.begin(), Dividers.end(), Dividers.begin(), FixDivider);
      return Error();
    }
    
  private:
    Receiver::Ptr Endpoint;
    boost::array<MultiFixed, InChannels> Matrix;
    MultiBigSample Dividers;
  };
}

namespace ZXTune
{
  namespace Sound
  {
    Error CreateMixer(uint_t channels, Mixer::Ptr& ptr)
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
