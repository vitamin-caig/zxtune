#include "filter.h"

#include <tools.h>

#include <boost/noncopyable.hpp>
#include <boost/static_assert.hpp>

#include <cassert>

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
  public:
    static const std::size_t MAX_ORDER = 
      std::numeric_limits<BigSample>::max() / (std::numeric_limits<C>::max() * std::numeric_limits<Sample>::max());
    FIRFilter(const C* coeffs, std::size_t order)
      : Matrix(coeffs, coeffs + order), Delegate(), History(order), Position(&History[0], &History[order])
    {
    }

    virtual void ApplySample(const Sample* input, std::size_t channels)
    {
      if (Receiver::Ptr delegate = Delegate)
      {
        assert(channels == ArraySize(Position->Array) || !"Invalid input channels for FIR filter");
        std::memcpy(&*Position, input, sizeof(SampleArray));
        BigSampleArray res = {0};

        for (std::vector<C>::const_iterator it = Matrix.begin(), lim = Matrix.end(); it != lim; ++it, --Position)
        {
          for (std::size_t chan = 0; chan != OUTPUT_CHANNELS; ++chan)
          {
            res[chan] += BigSample(*it) * Position->Array[chan];
          }
        }
        std::transform(res, ArrayEnd(res), Result, std::bind2nd(std::divides<BigSample>(), BigSample(FIXED_POINT_PRECISION)));
        ++Position;
        return delegate->ApplySample(Result, OUTPUT_CHANNELS);
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
    std::vector<C> Matrix;
    Receiver::Ptr Delegate;
    std::vector<SampleHelper> History;
    cycled_iterator<SampleHelper*> Position;
    SampleArray Result;
  };
}

namespace ZXTune
{
  namespace Sound
  {
    Convertor::Ptr CreateFIRFilter(const signed* coeffs, std::size_t order)
    {
      return Convertor::Ptr(new FIRFilter<signed>(coeffs, order));
    }

    void CalculateFIRCoefficients(std::size_t /*order*/, uint32_t /*freq*/,
      uint32_t /*lowCutoff*/, uint32_t /*highCutoff*/, std::vector<signed>& /*coeffs*/)
    {
    }
  }
}
