#include "filter.h"

namespace
{
  using namespace ZXTune::Sound;

  class FIRFilter : public Receiver
  {
  public:
    FIRFilter(const Sample* coeffs, std::size_t order, Receiver* delegate)
      : Matrix(coeffs, coeffs + order), Order(order), Delegate(delegate), History(Order), Position(0)
    {
    }

    virtual void ApplySample(const Sample* input, std::size_t channels)
    {
      assert(channels == OUTPUT_CHANNELS || !"Invalid input channels for FIR filter");
      const HistoryItem* const histFirst(&History[0]);
      const HistoryItem* const histLast(&History[Order]);
      const HistoryItem* const histCur(&History[Position]);
      HistoryItem* histPos(histCur);
      std::memcpy(histPos, input, sizeof(*histCur));
      BigSample res[Channels] = {0};
      for (const Sample* it = &Matrix[0], lim = &Matrix[Order]; it != lim; ++it, --histPos)
      {
        for (std::size_t chan = 0; chan != Channels; ++chan)
        {
          res[chan] += (*histPos)[chan] * *it;
        }
        if (histFirst == histPos)
        {
          histPos = histLast;
        }
      }
      for (std::size_t chan = 0; chan != OUTPUT_CHANNELS; ++chan)
      {
        Result[chan] = static_cast<Sample>(res[chan] / FIXED_POINT_PRECISION);
      }
      Position = (Position + 1) % Order;
      return Delegate->ApplySample(Result, Channels);
    }
  private:
    typedef Sample[OUTPUT_CHANNELS] HistoryItem;
    typedef std::vector<HistoryItem> HistoryArray;
    std::vector<Sample> Matrix;
    const std::size_t Order;
    Receiver* const Delegate;
    HistoryArray History;
    std::size_t Position;
    Sample Result[Channels];
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
