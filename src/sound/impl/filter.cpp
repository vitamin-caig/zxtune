/*
Abstract:
  FIR-filter implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/
#include "../filter.h"
#include "../error_codes.h"

#include "internal_types.h"

#include <tools.h>
#include <error.h>

#include <boost/noncopyable.hpp>
#include <boost/static_assert.hpp>

#include <cmath>
#include <limits>
#include <cassert>

#include <text/sound.h>

#define FILE_TAG 8A9585D9

namespace
{
  using namespace ZXTune::Sound;

  template<class IntSample>
  class FIRFilter : public Converter, private boost::noncopyable
  {
    typedef int64_t BigSample;
    typedef boost::array<BigSample, OUTPUT_CHANNELS> MultiBigSample;
    typedef std::vector<BigSample> MatrixType;
    
    typedef boost::array<IntSample, OUTPUT_CHANNELS> MultiIntSample;
    
    inline static IntSample Normalize(Sample smp)
    {
      return IntSample(smp) - SAMPLE_MID;
    }
    
    inline static Sample Denormalize(IntSample smp)
    {
      return smp + SAMPLE_MID;
    }
    
    inline static Sample Integral2Sample(BigSample smp)
    {
      return Denormalize(IntSample(smp / FIXED_POINT_PRECISION));
    }
  public:
    static const unsigned MIN_ORDER = 2;
    static const unsigned MAX_ORDER = 1u <<
      8 * (sizeof(BigSample) - sizeof(IntSample) - sizeof(Sample));
    explicit FIRFilter(const std::vector<IntSample>& coeffs)
      : Matrix(coeffs.begin(), coeffs.end()), Delegate()
      , History(coeffs.size()), Position(&History[0], &History.back() + 1)
    {
    }

    virtual void ApplySample(const MultiSample& data)
    {
      if (Delegate)
      {
        {
          std::transform(data.begin(), data.end(), Position->begin(), Normalize);
          
          MultiBigSample res = { {0} };

          for (typename MatrixType::const_iterator it = Matrix.begin(), lim = Matrix.end(); it != lim; ++it, --Position)
          {
            const typename MatrixType::value_type val(*it);
            const MultiIntSample& src(*Position);
            for (unsigned chan = 0; chan != OUTPUT_CHANNELS; ++chan)
            {
              res[chan] += val * src[chan];
            }
          }
          std::transform(res.begin(), res.end(), Result.begin(), Integral2Sample);
        }
        ++Position;
        return Delegate->ApplySample(Result);
      }
    }

    virtual void Flush()
    {
      if (Delegate)
      {
        return Delegate->Flush();
      }
    }

    virtual void SetEndpoint(Receiver::Ptr delegate)
    {
      Delegate = delegate;
    }
  private:
    MatrixType Matrix;
    Receiver::Ptr Delegate;
    std::vector<MultiIntSample> History;
    cycled_iterator<MultiIntSample*> Position;
    MultiSample Result;
  };

  //TODO: use from boost
  inline double bessel(double alpha)
  {
    const double delta = 1e-14;
    double term = 0.5;
    double f = 1.0;
    std::size_t k = 0;
    double result = 0.0;
    while (term < -delta || term > delta)
    {
      ++k;           //step
      f *= (alpha / 2) / k;  //f(k+1) = f(k)*(c/k),f(0)=1 c=alpha/2
      term = f * f;
      result += term;
    }
    return result;
  }

  //kaiser implementation
  inline void DoFFT(const double alpha, std::vector<double>& coeffs)
  {
    const unsigned order = unsigned(coeffs.size());
    const double denom = bessel(alpha);
    const double center = double(order - 1) / 2;
    for (unsigned tap = 0; tap < order; ++tap)
    {
      const double kg = (double(tap) - center) / center;
      const double kd = alpha * sqrt(1.0 - kg * kg);
      coeffs[tap] *= bessel(kd) / denom;
    }
  }

  template<class T>
  inline void CheckParams(T val, T min, T max, Error::LocationRef loc, const Char* text)
  {
    if (!in_range<T>(val, min, max))
    {
      throw MakeFormattedError(loc, FILTER_INVALID_PARAMS, text, val, min, max);
    }
  }
}

namespace ZXTune
{
  namespace Sound
  {
    Converter::Ptr CreateFIRFilter(const std::vector<signed>& coeffs)
    {
      return Converter::Ptr(new FIRFilter<signed>(coeffs));
    }

    void CalculateBandpassFilter(unsigned freq,
      unsigned lowCutoff, unsigned highCutoff, std::vector<signed>& coeffs)
    {
      //input parameters
      //gain = 10 ^^ (dB / 20)
      const Gain PASSGAIN = 1.0, STOPGAIN = 0;

      //check parameters
      const unsigned order = unsigned(coeffs.size());
      CheckParams(order, FIRFilter<signed>::MIN_ORDER, FIRFilter<signed>::MAX_ORDER, THIS_LINE, TEXT_SOUND_ERROR_FILTER_ORDER);
      CheckParams(highCutoff, unsigned(freq / order), freq / 2, THIS_LINE, TEXT_SOUND_ERROR_FILTER_HIGH_CUTOFF);
      CheckParams(lowCutoff, 0u, highCutoff, THIS_LINE, TEXT_SOUND_ERROR_FILTER_LOW_CUTOFF);

      //create freq responses
      std::vector<Gain> freqResponse(order, 0.0);
      const unsigned midOrder(order / 2);
      for (unsigned tap = 0; tap < midOrder; ++tap)
      {
        const unsigned tapFreq = unsigned(uint64_t(freq) * (tap + 1) / order);
        freqResponse[tap] = freqResponse[order - tap - 1] =
          (tapFreq < lowCutoff || tapFreq > highCutoff) ? STOPGAIN : PASSGAIN;
      }

      //transform coeffs from freq response
      std::vector<double> firCoeffs(order, 0.0);
      for (unsigned tap = 0; tap < midOrder; ++tap)
      {
        double tmpCoeff = 0.0;
        for (unsigned subtap = 0; subtap < order; ++subtap)
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
      std::transform(firCoeffs.begin(), firCoeffs.end(), coeffs.begin(), Gain2Fixed<signed>);
    }
  }
}
