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
    
  class MixerCore
  {
  public:
    typedef std::auto_ptr<MixerCore> Ptr;
    
    virtual ~MixerCore() {}
    virtual void ApplySample(const std::vector<Sample>& /*input*/, Receiver& /*rcv*/) const {}
  };
  
  template<unsigned InChannels>
  class FastMixerCore : public MixerCore, private boost::noncopyable
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
    explicit FastMixerCore(const std::vector<MultiGain>& matrix)
    {
      assert(matrix.size() == InChannels);
      std::transform(matrix.begin(), matrix.end(), Matrix.begin(), MultiGain2MultiFixed<NativeType>);
    }

    virtual void ApplySample(const std::vector<Sample>& inData, Receiver& rcv) const
    {
      if (inData.size() != InChannels)
      {
        throw MakeFormattedError(THIS_LINE, MIXER_CHANNELS_MISMATCH, 
          TEXT_SOUND_ERROR_MIXER_CHANNELS_MISMATCH, inData.size(), InChannels);
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
      return rcv.ApplySample(result);
    }
  private:
    boost::array<MultiFixed, InChannels> Matrix;
  };
  
  MixerCore::Ptr CreateMixerCore(const std::vector<MultiGain>& data)
  {
    switch (const unsigned size = unsigned(data.size()))
    {
    case 1:
      return MixerCore::Ptr(new FastMixerCore<1>(data));
    case 2:
      return MixerCore::Ptr(new FastMixerCore<2>(data));
    case 3:
      return MixerCore::Ptr(new FastMixerCore<3>(data));
    case 4:
      return MixerCore::Ptr(new FastMixerCore<4>(data));
    default:
      throw MakeFormattedError(THIS_LINE, MIXER_UNSUPPORTED, TEXT_SOUND_ERROR_MIXER_UNSUPPORTED, size);
    }
  }
  
  class MixerImpl : public Mixer
  {
  public:
    explicit MixerImpl(Receiver::Ptr endpoint)
      : Endpoint(endpoint), Core(new MixerCore())
    {
    }
    
    virtual void ApplySample(const std::vector<Sample>& data)
    {
      return Core->ApplySample(data, *Endpoint);
    }
    
    virtual void Flush()
    {
      return Endpoint->Flush();
    }
    
    virtual void SetMatrix(const std::vector<MultiGain>& data)
    {
      const std::vector<MultiGain>::const_iterator it(std::find_if(data.begin(), data.end(),
        FindOverloadedGain));
      if (it != data.end())
      {
        throw Error(THIS_LINE, MIXER_INVALID_MATRIX, TEXT_SOUND_ERROR_MIXER_INVALID_MATRIX);
      }
      Core = CreateMixerCore(data);
    }
  private:
    const Receiver::Ptr Endpoint;
    MixerCore::Ptr Core;
  };
}

namespace ZXTune
{
  namespace Sound
  {
    Mixer::Ptr Mixer::Create(Receiver::Ptr endpoint)
    {
      return Mixer::Ptr(new MixerImpl(endpoint));
    }
  }
}
