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
#include <math/numeric.h>
#include <sound/matrix_mixer.h>
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
  
  inline bool IsOverloadedGain(const MultiGain& mg)
  {
    return mg.end() != std::find_if(mg.begin(), mg.end(), !boost::bind(&Math::InRange<Gain>, _1, 0.0f, 1.0f));
  }
   
  template<unsigned Channels>
  class FastMixer : public FixedChannelsMatrixMixer<Channels>
  {
    //determine type for intermediate value
    static const uint_t INTERMEDIATE_BITS_MIN =
      SignificantBits<Sample>::Value +                   //input sample
      boost::static_log2<FIXED_POINT_PRECISION>::value + //mixer
      boost::static_log2<Channels>::value;             //channels count

    // calculate most suitable type for intermediate value storage
    typedef typename boost::mpl::if_c<
      INTERMEDIATE_BITS_MIN <= SignificantBits<NativeType>::Value,
      NativeType,
      MaxBigType
    >::type BigSample;
    typedef boost::array<BigSample, OUTPUT_CHANNELS> MultiBigSample;
    
    typedef boost::array<NativeType, OUTPUT_CHANNELS> MultiFixed;

    typedef FixedChannelsMatrixMixer<Channels> Base;
  public:
    FastMixer()
      : Endpoint(Receiver::CreateStub())
    {
      const NativeType val = static_cast<NativeType>(FIXED_POINT_PRECISION / Channels);
      for (uint_t inChan = 0; inChan != Channels; ++inChan)
      {
        MultiFixed& out = Matrix[inChan];
        for (uint_t outChan = 0; outChan != out.size(); ++outChan)
        {
          out[outChan] = val;
        }
      }
    }

    virtual void ApplyData(const typename Base::InDataType& inData)
    {
      // pass along input channels due to input data structure
      MultiBigSample res = { {0} };
      for (uint_t inChan = 0; inChan != Channels; ++inChan)
      {
        const NativeType in = NativeType(inData[inChan]) - SAMPLE_MID;
        const MultiFixed& inChanMix = Matrix[inChan];
        for (uint_t outChan = 0; outChan != res.size(); ++outChan)
        {
          res[outChan] += inChanMix[outChan] * in;
        }
      }
      OutputSample result;
      for (uint_t outChan = 0; outChan != result.size(); ++outChan)
      {
        result[outChan] = static_cast<Sample>(res[outChan] / FIXED_POINT_PRECISION + SAMPLE_MID);
      }
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
    
    virtual void SetMatrix(const typename Base::Matrix& data)
    {
      const typename Base::Matrix::const_iterator it = std::find_if(data.begin(), data.end(), &IsOverloadedGain);
      if (it != data.end())
      {
        throw Error(THIS_LINE, translate("Failed to set mixer matrix: gain is out of range."));
      }
      for (uint_t inChan = 0; inChan != Channels; ++inChan)
      {
        const MultiGain& in = data[inChan];
        MultiFixed& out = Matrix[inChan];
        for (uint_t outChan = 0; outChan != out.size(); ++outChan)
        {
          out[outChan] = Gain2Fixed<NativeType>(in[outChan] / Channels);
        }
      }
    }
  private:
    Receiver::Ptr Endpoint;
    boost::array<MultiFixed, Channels> Matrix;
  };
}

namespace ZXTune
{
  namespace Sound
  {
    template<>
    FixedChannelsMatrixMixer<1>::Ptr FixedChannelsMatrixMixer<1>::Create()
    {
      return boost::make_shared<FastMixer<1> >();
    }

    template<>
    FixedChannelsMatrixMixer<2>::Ptr FixedChannelsMatrixMixer<2>::Create()
    {
      return boost::make_shared<FastMixer<2> >();
    }

    template<>
    FixedChannelsMatrixMixer<3>::Ptr FixedChannelsMatrixMixer<3>::Create()
    {
      return boost::make_shared<FastMixer<3> >();
    }

    template<>
    FixedChannelsMatrixMixer<4>::Ptr FixedChannelsMatrixMixer<4>::Create()
    {
      return boost::make_shared<FastMixer<4> >();
    }
  }
}
