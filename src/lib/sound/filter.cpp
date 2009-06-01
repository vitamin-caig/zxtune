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

  class FIRFilter : public Convertor, private boost::noncopyable
  {
  public:
    FIRFilter(const Sample* coeffs, std::size_t order)
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

        for (std::vector<Sample>::const_iterator it = Matrix.begin(), lim = Matrix.end(); it != lim; ++it, --Position)
        {
          for (std::size_t chan = 0; chan != OUTPUT_CHANNELS; ++chan)
          {
            res[chan] += Position->Array[chan] * *it;
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
    std::vector<Sample> Matrix;
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
    Convertor::Ptr CreateFIRFilter(const Sample* coeffs, std::size_t order)
    {
      return Convertor::Ptr(new FIRFilter(coeffs, order));
    }

    void CalculateFIRCoefficients(std::size_t /*order*/, uint32_t /*freq*/,
      uint32_t /*lowCutoff*/, uint32_t /*highCutoff*/, std::vector<Sample>& /*coeffs*/)
    {
    }
  }
}
