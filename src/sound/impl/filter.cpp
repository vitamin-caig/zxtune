/*
Abstract:
  FIR-filter implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "internal_types.h"
//common includes
#include <error_tools.h>
#include <iterator.h>
#include <tools.h>
//library includes
#include <l10n/api.h>
#include <sound/filter.h>
//std includes
#include <cassert>
#include <cmath>
#include <limits>
#include <numeric>
//boost includes
#include <boost/noncopyable.hpp>
#include <boost/static_assert.hpp>
#include <boost/integer/static_log2.hpp>
#include <boost/type_traits/is_signed.hpp>

#define FILE_TAG 8A9585D9

namespace
{
  using namespace ZXTune::Sound;

  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("sound");

  //TODO: use from boost
  inline double bessel(double alpha)
  {
    const double delta = 1e-14;
    double term = 0.5;
    double f = 1.0;
    uint_t k = 0;
    double result = 0.0;
    while (term < -delta || term > delta)
    {
      ++k;                   //step
      f *= (alpha / 2) / k;  //f(k+1) = f(k)*(c/k),f(0)=1 c=alpha/2
      term = f * f;
      result += term;
    }
    return result;
  }

  //kaiser implementation
  inline void DoFFT(const double alpha, std::vector<double>& coeffs)
  {
    const uint_t order = static_cast<uint_t>(coeffs.size());
    const double denom = bessel(alpha);
    const double center = double(order - 1) / 2;
    for (uint_t tap = 0; tap < order; ++tap)
    {
      const double kg = (double(tap) - center) / center;
      const double kd = alpha * sqrt(1.0 - kg * kg);
      coeffs[tap] *= bessel(kd) / denom;
    }
  }

  template<class T>
  inline void CheckParams(T val, T min, T max, Error::LocationRef loc, const String& text)
  {
    if (!in_range<T>(val, min, max))
    {
      throw MakeFormattedError(loc, text, val, min, max);
    }
  }

  template<class T>
  struct SignificantBits
  {
    static const uint_t Value = boost::is_signed<T>::value ? 8 * sizeof(T) - 1 : 8 * sizeof(T);
  };

  template<class IntSample, class BigSample>
  class FIRFilter : public Filter, private boost::noncopyable
  {
    //intermediate type
    typedef boost::array<BigSample, OUTPUT_CHANNELS> MultiBigSample;
    //main working type
    typedef boost::array<IntSample, OUTPUT_CHANNELS> MultiIntSample;
    typedef std::vector<IntSample> MatrixType;
    
    inline static Sample Integral2Sample(BigSample smp)
    {
      return static_cast<Sample>(smp / FIXED_POINT_PRECISION + SAMPLE_MID);
    }
  public:
    static const uint_t MIN_ORDER = 2;
    //all bits = SampleBits + 1 + FixedBits + 1 + OrderBits
    static const uint_t MAX_ORDER = uint_t(1) <<
      (SignificantBits<BigSample>::Value - (SignificantBits<Sample>::Value + 1) - (boost::static_log2<FIXED_POINT_PRECISION>::value + 1));
      
    explicit FIRFilter(uint_t order)
      : Matrix(order), Delegate(Receiver::CreateStub())
      , History(order), Position(&History[0], &History.back() + 1)
    {
      assert(in_range<uint_t>(order, MIN_ORDER, MAX_ORDER));
    }

    virtual void ApplyData(const MultiSample& data)
    {
      std::transform(data.begin(), data.end(), Position->begin(), std::bind2nd(std::minus<IntSample>(), SAMPLE_MID));
      MultiBigSample res = { {0} };

      for (typename MatrixType::const_iterator it = Matrix.begin(), lim = Matrix.end(); it != lim; ++it, --Position)
      {
        const typename MatrixType::value_type val = *it;
        const MultiIntSample& src = *Position;
        for (uint_t chan = 0; chan != OUTPUT_CHANNELS; ++chan)
        {
          res[chan] += val * src[chan];
        }
      }
      MultiSample result;
      std::transform(res.begin(), res.end(), result.begin(), &Integral2Sample);
      ++Position;
      return Delegate->ApplyData(result);
    }

    virtual void Flush()
    {
      Delegate->Flush();
    }

    virtual void SetTarget(Receiver::Ptr delegate)
    {
      Delegate = delegate ? delegate : Receiver::CreateStub();
    }
    
    virtual Error SetBandpassParameters(uint_t freq, uint_t lowCutoff, uint_t highCutoff)
    {
      try
      {
        //input parameters
        //gain = 10 ^^ (dB / 20)
        const Gain PASSGAIN = 0.90, STOPGAIN = 0;

        //check parameters
        const uint_t order = static_cast<uint_t>(Matrix.size());
        CheckParams(order, MIN_ORDER, MAX_ORDER, THIS_LINE, translate("Specified filter order (%1%) is out of range (%2%...%3%)."));
        CheckParams(highCutoff, freq / order, freq / 2, THIS_LINE, translate("Specified filter high cutoff frequency (%1%Hz) is out of range (%2%...%3%Hz)."));
        CheckParams(lowCutoff, uint_t(0), highCutoff, THIS_LINE, translate("Specified filter low cutoff frequency (%1%Hz) is out of range (%2%...%3%Hz)."));

        //create freq responses
        std::vector<Gain> freqResponse(order, 0.0);
        const uint_t midOrder = order / 2;
        for (uint_t tap = 0; tap < midOrder; ++tap)
        {
          const uint_t tapFreq = static_cast<uint_t>(uint64_t(freq) * (tap + 1) / order);
          freqResponse[tap] = freqResponse[order - tap - 1] =
            (tapFreq < lowCutoff || tapFreq > highCutoff) ? STOPGAIN : PASSGAIN;
        }

        //transform coeffs from freq response
        std::vector<double> firCoeffs(order, 0.0);
        for (uint_t tap = 0; tap < midOrder; ++tap)
        {
          double tmpCoeff = 0.0;
          for (uint_t subtap = 0; subtap < order; ++subtap)
          {
            const double omega = 2.0 * 3.14159265358 * tap * subtap / order;
            tmpCoeff += freqResponse[subtap] * cos(omega);
          }
          firCoeffs[midOrder - tap] = firCoeffs[midOrder + tap] = tmpCoeff / order;
        }
        //do FFT transformation
        const double ALPHA = 8.0;
        DoFFT(ALPHA, firCoeffs);
        //put result
        std::transform(firCoeffs.begin(), firCoeffs.end(), Matrix.begin(), Gain2Fixed<IntSample>);
        //reset
        History.clear();
        History.resize(order);
        Position = CycledIterator<MultiIntSample*>(&History[0], &History.back() + 1);
        return Error();
      }
      catch (const Error& e)
      {
        return e;
      }
    }
  private:
    MatrixType Matrix;
    Receiver::Ptr Delegate;
    std::vector<MultiIntSample> History;
    CycledIterator<MultiIntSample*> Position;
  };
}

namespace ZXTune
{
  namespace Sound
  {
    Error CreateFIRFilter(uint_t order, Filter::Ptr& result)
    {
      typedef FIRFilter<int_t, int_t> FIRFilterType;
    
      try
      {
        CheckParams(order, FIRFilterType::MIN_ORDER, FIRFilterType::MAX_ORDER, THIS_LINE, translate("Specified filter order (%1%) is out of range (%2%...%3%)."));
        result.reset(new FIRFilterType(order));
        return Error();
      }
      catch (const Error& e)
      {
        return e;
      }
    }
  }
}
