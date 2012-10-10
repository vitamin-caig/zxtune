/*
Abstract:
  Mixer implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "internal_types.h"
//common includes
#include <tools.h>
#include <error_tools.h>
//library includes
#include <l10n/api.h>
#include <sound/mixer.h>
//std includes
#include <algorithm>
#include <numeric>
//boost includes
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/noncopyable.hpp>
#include <boost/integer/static_log2.hpp>
#include <boost/mpl/if.hpp>
#include <boost/type_traits/is_signed.hpp>

#define FILE_TAG 278565B1

namespace
{
  using namespace ZXTune::Sound;

  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("sound");

  //using unsigned as a native type gives better performance (very strange...), at least at gcc
  typedef int NativeType;
  typedef int64_t MaxBigType;

  template<class T>
  struct SignificantBits
  {
    static const uint_t Value = boost::is_signed<T>::value ? 8 * sizeof(T) - 1 : 8 * sizeof(T);
  };

  const MaxBigType MAX_MIXER_CHANNELS = MaxBigType(1) << (SignificantBits<MaxBigType>::Value - SignificantBits<Sample>::Value -
    boost::static_log2<FIXED_POINT_PRECISION>::value);
  
  inline bool FindOverloadedGain(const MultiGain& mg)
  {
    return mg.end() != std::find_if(mg.begin(), mg.end(), !boost::bind(in_range<Gain>, _1, 0.0f, 1.0f));
  }
   
  template<uint_t InChannels>
  class FastMixer : public MatrixMixer, private boost::noncopyable
  {
    //determine type for intermediate value
    static const uint_t INTERMEDIATE_BITS_MIN =
      SignificantBits<Sample>::Value +                   //input sample
      boost::static_log2<FIXED_POINT_PRECISION>::value + //mixer
      boost::static_log2<InChannels>::value;             //channels count

    // calculate most suitable type for intermediate value storage
    typedef typename boost::mpl::if_c<
      INTERMEDIATE_BITS_MIN <= SignificantBits<NativeType>::Value,
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

    static inline Sample NormalizeSum(BigSample in, BigSample divider)
    {
      return static_cast<Sample>(in / divider + SAMPLE_MID);
    }
  public:
    FastMixer()
      : Endpoint(Receiver::CreateStub())
    {
      std::fill(Matrix.front().begin(), Matrix.front().end(), static_cast<NativeType>(FIXED_POINT_PRECISION));
      std::fill(Matrix.begin(), Matrix.end(), Matrix.front());
      std::fill(Dividers.begin(), Dividers.end(), BigSample(FIXED_POINT_PRECISION * InChannels));
    }

    virtual void ApplyData(const std::vector<Sample>& inData)
    {
      assert(inData.size() == InChannels || !"Mixer::ApplyData channels mismatch");
      // pass along input channels due to input data structure
      MultiBigSample res = { {0} };
      for (uint_t inChan = 0; inChan != InChannels; ++inChan)
      {
        const NativeType in = NativeType(inData[inChan]) - SAMPLE_MID;
        const MultiFixed& inChanMix = Matrix[inChan];
        for (uint_t outChan = 0; outChan != OUTPUT_CHANNELS; ++outChan)
        {
          res[outChan] += inChanMix[outChan] * in;
        }
      }
      MultiSample result;
      std::transform(res.begin(), res.end(), Dividers.begin(), result.begin(), &NormalizeSum);
      return Endpoint->ApplyData(result);
    }
    
    virtual void Flush()
    {
      Endpoint->Flush();
    }
    
    virtual void SetTarget(Receiver::Ptr rcv)
    {
      Endpoint = rcv ? rcv : Receiver::CreateStub();
    }
    
    virtual void SetMatrix(const std::vector<MultiGain>& data)
    {
      if (data.size() != InChannels)
      {
        throw Error(THIS_LINE, translate("Failed to set mixer matrix: invalid channels count specified."));
      }
      const std::vector<MultiGain>::const_iterator it = std::find_if(data.begin(), data.end(),
        FindOverloadedGain);
      if (it != data.end())
      {
        throw Error(THIS_LINE, translate("Failed to set mixer matrix: gain is out of range."));
      }
      boost::array<MultiFixed, InChannels> tmpMatrix;
      std::transform(data.begin(), data.end(), tmpMatrix.begin(), MultiGain2MultiFixed<NativeType>);
      MultiBigSample tmpDividers = std::accumulate(tmpMatrix.begin(), tmpMatrix.end(), MultiBigSample(), AddBigsamples);
      // prevent empty dividers
      std::transform(tmpDividers.begin(), tmpDividers.end(), tmpDividers.begin(), FixDivider);
      Matrix.swap(tmpMatrix);
      Dividers.swap(tmpDividers);
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
    MatrixMixer::Ptr CreateMatrixMixer(uint_t channels)
    {
      switch (channels)
      {
      case 1:
        return boost::make_shared<FastMixer<1> >();
      case 2:
        return boost::make_shared<FastMixer<2> >();
      case 3:
        return boost::make_shared<FastMixer<3> >();
      case 4:
        return boost::make_shared<FastMixer<4> >();
      default:
        throw MakeFormattedError(THIS_LINE, translate("Failed to create unsupported mixer with %1% channels."), channels);
      }
    }
  }
}
