#include "filter.h"

#include <tools.h>

#include <cassert>

namespace
{
  using namespace ZXTune::Sound;

  struct SampleHelper
  {
    SampleArray Array;
  };

  template<class C>
  class cycled_iterator
  {
  public:
    cycled_iterator(C* start, C* stop) : begin(start), end(stop), cur(start)
    {
    }

    cycled_iterator(const cycled_iterator& rh) : begin(rh.begin), end(rh.end), cur(rh.cur)
    {
    }

    cycled_iterator& operator ++ ()
    {
      if (end == ++cur)
      {
        cur = begin;
      }
      return *this;
    }

    cycled_iterator& operator -- ()
    {
      if (begin == cur)
      {
        cur = end;
      }
      --cur;
      return *this;
    }

    C& operator * () const
    {
      return *cur;
    }

    C* operator ->() const
    {
      return cur;
    }
  private:
    C* const begin;
    C* const end;
    C* cur;
  };

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
  private:
    std::vector<Sample> Matrix;
    Receiver* const Delegate;
    std::vector<SampleHelper> History;
    cycled_iterator<SampleHelper> Position;
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
  }
}
