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

  template<class C>
  class FIRFilter : public ChainedReceiver, private boost::noncopyable
  {
    typedef int64_t BigSample;
    typedef BigSample BigSampleArray[OUTPUT_CHANNELS];
    typedef std::vector<BigSample> MatrixType;
  public:
    static const std::size_t MAX_ORDER = 1 <<
      8 * (sizeof(BigSample) - sizeof(C) - sizeof(Sample));
    explicit FIRFilter(const std::vector<C>& coeffs)
      : Matrix(coeffs.begin(), coeffs.end()), Delegate()
      , History(coeffs.size()), Position(&History[0], &History.back() + 1)
    {
    }

    virtual void ApplySample(const Sample* input, unsigned channels)
    {
      if (Receiver::Ptr delegate = Delegate.lock())
      {
        if (channels != ArraySize(Position->Data))
	{
	  throw MakeFormattedError(THIS_LINE, CHANNELS_MISMATCH, TEXT_SOUND_ERROR_CHANNELS_MISMATCH, "filter", channels, ArraySize(Position->Data));
	}
        std::memcpy(Position->Data, input, sizeof(Position->Data));
        BigSampleArray res = {0};

        for (typename MatrixType::const_iterator it = Matrix.begin(), lim = Matrix.end(); it != lim; ++it, --Position)
        {
          for (std::size_t chan = 0; chan != OUTPUT_CHANNELS; ++chan)
          {
            res[chan] += *it * Position->Data[chan];
          }
        }
        std::transform(res, ArrayEnd(res), Result, std::bind2nd(std::divides<BigSample>(), BigSample(FIXED_POINT_PRECISION)));
        ++Position;
        return delegate->ApplySample(Result, channels);
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
    MatrixType Matrix;
    Receiver::WeakPtr Delegate;
    std::vector<Multisample> History;
    cycled_iterator<Multisample*> Position;
    SampleArray Result;
  };

  //TODO: use from boost
  double bessel(double alpha)
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
  void DoFFT(const double alpha, std::vector<double>& coeffs)
  {
    const double denom = bessel(alpha);
    const double center = double(coeffs.size() - 1) / 2;
    for (std::size_t tap = 0; tap < coeffs.size(); ++tap)
    {
      const double kg = (double(tap) - center) / center;
      const double kd = alpha * sqrt(1.0 - kg * kg);
      coeffs[tap] *= bessel(kd) / denom;
    }
  }

  signed DoubleToSigned(double val)
  {
    return static_cast<signed>(val * FIXED_POINT_PRECISION);
  }
  
  template<class T>
  void CheckParams(T val, T limit, Error::LocationRef loc, const Char* text)
  {
    if (val > limit)
    {
      throw MakeFormattedError(loc, FILTER_INVALID_PARAMS, text, val, limit);
    }
  }
}

namespace ZXTune
{
  namespace Sound
  {
    ChainedReceiver::Ptr CreateFIRFilter(const std::vector<signed>& coeffs)
    {
      return ChainedReceiver::Ptr(new FIRFilter<signed>(coeffs));
    }

    void CalculateBandpassFilter(unsigned freq,
      unsigned lowCutoff, unsigned highCutoff, std::vector<signed>& coeffs)
    {
      //input parameters
      //gain = 10 ^^ (dB / 20)
      const double PASSGAIN = 1.0, STOPGAIN = 0;
      const double PI = 3.14159265359;

      //check parameters
      const std::size_t order = coeffs.size();
      CheckParams(order, FIRFilter<signed>::MAX_ORDER, THIS_LINE, TEXT_SOUND_ERROR_FILTER_ORDER);
      CheckParams(highCutoff, freq / 2, THIS_LINE, TEXT_SOUND_ERROR_FILTER_HIGH_CUTOFF);
      CheckParams(lowCutoff, highCutoff, THIS_LINE, TEXT_SOUND_ERROR_FILTER_LOW_CUTOFF);

      //create freq responses
      std::vector<double> freqResponse(order, 0.0);
      const std::size_t midOrder(order / 2);
      for (std::size_t tap = 0; tap < midOrder; ++tap)
      {
        const unsigned tapFreq(freq * (tap + 1) / order);
        freqResponse[tap] = freqResponse[order - tap - 1] =
          (tapFreq < lowCutoff || tapFreq > highCutoff) ? STOPGAIN : PASSGAIN;
      }

      //transform coeffs from freq response
      std::vector<double> firCoeffs(order, 0.0);
      for (std::size_t tap = 0; tap < midOrder; ++tap)
      {
        double tmpCoeff = 0.0;
        for (std::size_t subtap = 0; subtap < order; ++subtap)
        {
          const double omega = 2.0 * PI * tap * subtap / order;
          tmpCoeff += freqResponse[subtap] * cos(omega);
        }
        firCoeffs[midOrder - tap] = firCoeffs[midOrder + tap] = tmpCoeff / order;
      }
      //do FFT transformation
      const double ALPHA = 8.0;
      DoFFT(ALPHA, firCoeffs);
      //put result
      std::transform(firCoeffs.begin(), firCoeffs.end(), coeffs.begin(), DoubleToSigned);
    }
  }
}
