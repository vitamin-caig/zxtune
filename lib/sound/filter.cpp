#include "filter.h"

#include <tools.h>

#include <cassert>
#include <boost/static_assert.hpp>

namespace
{
  using namespace ZXTune::Sound;

  struct SampleHelper
  {
    SampleArray Array;
  };

  BOOST_STATIC_ASSERT(sizeof(SampleHelper) == sizeof(SampleArray));

  class FIRFilter : public Receiver
  {
  public:
    FIRFilter(const Sample* coeffs, std::size_t order, Receiver* delegate)
      : Matrix(coeffs, coeffs + order), Delegate(delegate), History(order), Position(&History[0], &History[order])
    {
    }

    virtual void ApplySample(const Sample* input, std::size_t channels)
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
      for (std::size_t chan = 0; chan != OUTPUT_CHANNELS; ++chan)
      {
        Result[chan] = static_cast<Sample>(res[chan] / FIXED_POINT_PRECISION);
      }
      ++Position;
      return Delegate->ApplySample(Result, OUTPUT_CHANNELS);
    }

    virtual void Flush()
    {
      return Delegate->Flush();
    }

  private:
    std::vector<Sample> Matrix;
    Receiver* const Delegate;
    std::vector<SampleHelper> History;
    cycled_iterator<SampleHelper*> Position;
    SampleArray Result;
  };
}

namespace ZXTune
{
  namespace Sound
  {
    Receiver::Ptr CreateFIRFilter(const Sample* coeffs, std::size_t order, Receiver* delegate)
    {
      return Receiver::Ptr(new FIRFilter(coeffs, order, delegate));
    }

    void CalculateFIRCoefficients(std::size_t order, uint32_t freq, uint32_t lowCutoff, uint32_t highCutoff, std::vector<Sample>& coeffs)
    {
    }
  }
}
