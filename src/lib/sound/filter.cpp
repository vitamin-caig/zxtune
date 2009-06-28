#include "filter.h"

#include <tools.h>
#include <error.h>

#include <boost/noncopyable.hpp>
#include <boost/static_assert.hpp>

#include <cmath>
#include <limits>
#include <cassert>

#include <text/errors.h>

#define FILE_TAG 8A9585D9

namespace
{
  using namespace ZXTune::Sound;

  struct SampleHelper
  {
    SampleArray Array;
  };

  BOOST_STATIC_ASSERT(sizeof(SampleHelper) == sizeof(SampleArray));

  template<class C>
  class FIRFilter : public Convertor, private boost::noncopyable
  {
    typedef int64_t BigSample;
    typedef BigSample BigSampleArray[OUTPUT_CHANNELS];
    typedef std::vector<BigSample> MatrixType;
  public:
    static const std::size_t MAX_ORDER = 1 <<
      8 * (sizeof(BigSample) - sizeof(C) - sizeof(Sample));
    FIRFilter(const std::vector<C>& coeffs)
      : Matrix(coeffs.begin(), coeffs.end()), Delegate()
      , History(coeffs.size()), Position(&History[0], &History.back() + 1)
    {
    }

    virtual void ApplySample(const Sample* input, std::size_t channels)
    {
      if (Receiver::Ptr delegate = Delegate)
      {
        assert(channels == ArraySize(Position->Array) || !"Invalid input channels for FIR filter");
        std::memcpy(Position->Array, input, sizeof(SampleArray));
        BigSampleArray res = {0};

        for (typename MatrixType::const_iterator it = Matrix.begin(), lim = Matrix.end(); it != lim; ++it, --Position)
        {
          for (std::size_t chan = 0; chan != OUTPUT_CHANNELS; ++chan)
          {
            res[chan] += *it * Position->Array[chan];
          }
        }
        std::transform(res, ArrayEnd(res), Result, std::bind2nd(std::divides<BigSample>(), BigSample(FIXED_POINT_PRECISION)));
        ++Position;
        return delegate->ApplySample(Result, channels);
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
    MatrixType Matrix;
    Receiver::Ptr Delegate;
    std::vector<SampleHelper> History;
    cycled_iterator<SampleHelper*> Position;
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
}

namespace ZXTune
{
  namespace Sound
  {
    Convertor::Ptr CreateFIRFilter(const std::vector<signed>& coeffs)
    {
      return Convertor::Ptr(new FIRFilter<signed>(coeffs));
    }

    void CalculateFIRCoefficients(uint32_t freq,
      uint32_t lowCutoff, uint32_t highCutoff, std::vector<signed>& coeffs)
    {
      //input parameters
      //gain = 10 ^^ (dB / 20)
      const double PASSGAIN = 1.0, STOPGAIN = 0;
      const double PI = 3.14159265359;

      //check parameters
      const std::size_t order = coeffs.size();
      if (order > FIRFilter<signed>::MAX_ORDER)
      {
        throw Error(ERROR_DETAIL, 1, TEXT_ERROR_INVALID_FILTER);//TODO: code
      }
      highCutoff = std::min(highCutoff, freq / 2);
      lowCutoff = std::min(lowCutoff, highCutoff);

      //create freq responses
      std::vector<double> freqResponse(order, 0.0);
      const std::size_t midOrder(order / 2);
      for (std::size_t tap = 0; tap < midOrder; ++tap)
      {
        const uint32_t tapFreq(freq * (tap + 1) / order);
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
