#ifndef __FILTER_H_DEFINED__
#define __FILTER_H_DEFINED__

#include <sound.h>

namespace ZXTune
{
  namespace Sound
  {
    /*
      FIR filter with fixed-point calculations
    */
    template<std::size_t Channels, class C, typename C Precision>
    class FIRFilter : public Receiver
    {
    public:
      FIRFilter(const C* coeffs, std::size_t order, Receiver* delegate)
        : Matrix(coeffs, coeffs + order), Order(order), Delegate(delegate), History(Order), Position(0)
      {
      }

      virtual void ApplySample(const Sample* input, std::size_t channels)
      {
        assert(channels == Channels || !"Invalid input channels FIR filter specified");
        const HistoryItem* const histFirst(&History[0]);
        const HistoryItem* const histLast(&History[Order]);
        const HistoryItem* const histCur(&History[Position]);
        HistoryItem* histPos(histCur);
        std::memcpy(histPos, input, sizeof(*histCur));
        C res[Channels];
        for (const C* it = &Matrix[0], lim = &Matrix[Order]; it != lim; ++it, --histPos)
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
        for (std::size_t chan = 0; chan != Channels; ++chan)
        {
          Result[chan] = static_cast<Sample>(res[chan] / Precision);
        }
        Position = (Position + 1) % Order;
        return Delegate->ApplySample(Result, Channels);
      }
    private:
      typedef Sample[Channels] HistoryItem;
      typedef std::vector<HistoryItem> HistoryArray;
      std::vector<C> Matrix;
      const std::size_t Order;
      Receiver* const Delegate;
      HistoryArray History;
      std::size_t Position;
      Sample Result[Channels];
    };

    template<std::size_t Channels, class C = int32_t, typename C Precision = 100>
    inline Receiver::Ptr CreateFIRFilter(const C* coeffs, std::size_t order, Receiver* delegate)
    {
      return Receiver::Ptr(new FIRFilter<Channels, C, Precision>(coeffs, order, delegate));
    }
  }
}

#endif //__FILTER_H_DEFINED__
