/*
Abstract:
  FIR-filter implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/
#include "internal_types.h"

#include <tools.h>
#include <error_tools.h>
#include <sound/error_codes.h>
#include <sound/filter.h>

#include <boost/noncopyable.hpp>
#include <boost/static_assert.hpp>

#include <cassert>
#include <cmath>
#include <limits>
#include <numeric>

#include <text/sound.h>

#define FILE_TAG 8A9585D9

namespace
{
  using namespace ZXTune::Sound;

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
    const unsigned order = static_cast<unsigned>(coeffs.size());
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
      throw MakeFormattedError(loc, FILTER_INVALID_PARAMETER, text, val, min, max);
    }
  }

  //TODO: use IIR filter to remove DC
  template<class T>
  class Integrator
  {
  public:
    explicit Integrator(unsigned size)
      : Size(size), Buffer(size), Current(Buffer.begin()), Sum()
    {
    }

    T Update(T val)
    {
      Sum -= *Current;
      Sum += val;
      *Current = val;
      if (++Current == Buffer.end())
      {
        Current = Buffer.begin();
      }
      return Sum / Size;
    }
  private:
    const unsigned Size;
    std::vector<T> Buffer;
    typename std::vector<T>::iterator Current;
    T Sum;
  };
  
  template<class IntSample>
  class FIRFilter : public Filter, private boost::noncopyable
  {
    typedef int64_t BigSample;
    typedef boost::array<BigSample, OUTPUT_CHANNELS> MultiBigSample;
    typedef std::vector<BigSample> MatrixType;
    
    typedef boost::array<IntSample, OUTPUT_CHANNELS> MultiIntSample;
    
    inline static Sample Integral2Sample(BigSample smp, IntSample mid)
    {
      return IntSample(smp / FIXED_POINT_PRECISION) + mid;
    }
  public:
    static const unsigned MIN_ORDER = 2;
    static const unsigned MAX_ORDER = 1u <<
      8 * (sizeof(BigSample) - sizeof(IntSample) - sizeof(Sample));
      
    explicit FIRFilter(unsigned order)
      : Matrix(order), Delegate(CreateDummyReceiver())
      , History(order), Position(&History[0], &History.back() + 1)
      , Midval(4096)
    {
      assert(in_range<unsigned>(order, MIN_ORDER, MAX_ORDER));
    }

    virtual void ApplySample(const MultiSample& data)
    {
      std::copy(data.begin(), data.end(), Position->begin());
      const IntSample avg = Midval.Update(std::accumulate(Position->begin(), Position->end(), 0)) / OUTPUT_CHANNELS;
      std::transform(Position->begin(), Position->end(), Position->begin(), std::bind2nd(std::minus<IntSample>(), avg));
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
      MultiSample result;
      std::transform(res.begin(), res.end(), result.begin(), std::bind2nd(std::ptr_fun(&Integral2Sample), avg));
      ++Position;
      return Delegate->ApplySample(result);
    }

    virtual void Flush()
    {
      Delegate->Flush();
    }

    virtual void SetEndpoint(Receiver::Ptr delegate)
    {
      Delegate = delegate;
    }
    
    virtual Error SetBandpassParameters(unsigned freq, unsigned lowCutoff, unsigned highCutoff)
    {
      try
      {
        //input parameters
        //gain = 10 ^^ (dB / 20)
        const Gain PASSGAIN = 1.0, STOPGAIN = 0;

        //check parameters
        const unsigned order = static_cast<unsigned>(Matrix.size());
        CheckParams(order, MIN_ORDER, MAX_ORDER, THIS_LINE, TEXT_SOUND_ERROR_FILTER_ORDER);
        CheckParams(highCutoff, static_cast<unsigned>(freq / order), freq / 2, THIS_LINE, TEXT_SOUND_ERROR_FILTER_HIGH_CUTOFF);
        CheckParams(lowCutoff, 0u, highCutoff, THIS_LINE, TEXT_SOUND_ERROR_FILTER_LOW_CUTOFF);

        //create freq responses
        std::vector<Gain> freqResponse(order, 0.0);
        const unsigned midOrder(order / 2);
        for (unsigned tap = 0; tap < midOrder; ++tap)
        {
          const unsigned tapFreq = static_cast<unsigned>(uint64_t(freq) * (tap + 1) / order);
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
        std::transform(firCoeffs.begin(), firCoeffs.end(), Matrix.begin(), Gain2Fixed<IntSample>);
        //reset
        History.clear();
        History.resize(order);
        Position = cycled_iterator<MultiIntSample*>(&History[0], &History.back() + 1);
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
    cycled_iterator<MultiIntSample*> Position;
    Integrator<IntSample> Midval;
  };
}

namespace ZXTune
{
  namespace Sound
  {
    Error CreateFIRFilter(unsigned order, Filter::Ptr& result)
    {
      typedef FIRFilter<signed> FIRFilterType;
    
      try
      {
        CheckParams(order, FIRFilterType::MIN_ORDER, FIRFilterType::MAX_ORDER, THIS_LINE, TEXT_SOUND_ERROR_FILTER_ORDER);
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
