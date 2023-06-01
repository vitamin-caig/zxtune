/**
 *
 * @file
 *
 * @brief  FFT analyzer implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// common includes
#include <make_ptr.h>
// library includes
#include <sound/impl/fft_analyzer.h>
// std includes
#include <algorithm>
#include <array>
#include <complex>
#include <utility>

namespace Sound
{
  //! @brief %Sound analyzer interface
  class FFTAnalyzerImpl : public FFTAnalyzer
  {
  private:
    static const std::size_t WindowSizeLog = 10;
    static const std::size_t WindowSize = 1 << WindowSizeLog;

  public:
    void GetSpectrum(LevelType* result, std::size_t limit) const override
    {
      Produced = 0;
      FFT(result, limit);
    }

    void FeedSound(const Sample* samples, std::size_t count) override
    {
      const uint_t MAX_PRODUCED_DELTA = 10;
      if (Produced >= MAX_PRODUCED_DELTA)
      {
        return;
      }
      if (count >= WindowSize)
      {
        samples = samples + count - WindowSize;
        count = WindowSize;
      }
      for (auto *it = samples, *lim = samples + count; it != lim; ++it)
      {
        static_assert(Sound::Sample::MID == 0, "Incompatible sample type");
        const auto level = (it->Left() + it->Right()) / 2;
        Input[Cursor] = level;
        if (++Cursor == WindowSize)
        {
          Cursor = 0;
          ++Produced;
        }
      }
    }

  private:
    using Complex = std::complex<float>;

    class Lookup
    {
    private:
      Lookup()
      {
        for (std::size_t idx = 0; idx < BitRev.size(); ++idx)
        {
          BitRev[idx] = ReverseBits(idx);
        }
        for (std::size_t idx = 0; idx < Sinus.size(); ++idx)
        {
          const auto angle = GetAngle(idx);
          Sinus[idx] = std::sin(angle);
        }
        HammingWindow();
      }

      static float GetAngle(std::size_t idx)
      {
        return 2.0f * 3.14159265358f * idx / WindowSize;
      }

      // http://dspsystem.narod.ru/add/win/win.html
      void HammingWindow()
      {
        const auto a0 = 0.54f;
        const auto a1 = 0.46f;
        for (std::size_t idx = 0; idx < Window.size(); ++idx)
        {
          const auto angle = GetAngle(idx);
          Window[idx] = a0 - a1 * std::cos(angle);
        }
      }

      static const Lookup& Instance()
      {
        static const Lookup INSTANCE;
        return INSTANCE;
      }

      static uint_t ReverseBits(uint_t val)
      {
        uint_t reversed = 0;
        for (uint_t loop = 0; loop < WindowSizeLog; ++loop)
        {
          reversed <<= 1;
          reversed += (val & 1);
          val >>= 1;
        }
        return reversed;
      }

    public:
      static std::array<Complex, WindowSize> ToComplex(const std::array<int_t, WindowSize>& input, std::size_t offset)
      {
        const auto& self = Instance();
        std::array<Complex, WindowSize> result;
        for (std::size_t idx = 0; idx < WindowSize; ++idx)
        {
          result[idx] = self.Window[idx] * input[(offset + self.BitRev[idx]) % WindowSize];
        }
        return result;
      }

      static Complex Polar(uint_t val)
      {
        const auto& sinus = Instance().Sinus;
        return {sinus[(val + WindowSize / 4) % WindowSize], sinus[val]};
      }

    private:
      std::array<uint_t, WindowSize> BitRev;
      std::array<float, WindowSize> Sinus;
      std::array<float, WindowSize> Window;
    };

    void FFT(LevelType* result, std::size_t limit) const
    {
      auto cplx = Lookup::ToComplex(Input, Cursor);
      uint_t exchanges = 1;
      uint_t factFact = WindowSize / 2;
      for (uint_t i = WindowSizeLog; i != 0; --i, exchanges *= 2, factFact /= 2)
      {
        for (uint_t j = 0; j != exchanges; ++j)
        {
          const auto fact = Lookup::Polar(j * factFact);
          for (uint_t k = j; k < WindowSize; k += exchanges * 2)
          {
            const auto k1 = k + exchanges;
            const auto tmp = fact * cplx[k1];
            cplx[k1] = cplx[k] - tmp;
            cplx[k] += tmp;
          }
        }
      }

      const std::size_t toFill = std::min(limit, WindowSize / 2);
      for (std::size_t i = 0; i < toFill; ++i)
      {
        const uint_t LIMIT = LevelType::PRECISION;
        const uint_t raw = std::abs(cplx[i + 1]) / (256 * 32);
        result[i] = LevelType(std::min(raw, LIMIT), LIMIT);
      }
      std::fill_n(result + toFill, limit - toFill, LevelType());
    }

  private:
    mutable uint_t Produced = 0;
    std::array<int_t, WindowSize> Input;
    std::size_t Cursor = 0;
  };

  FFTAnalyzer::Ptr FFTAnalyzer::Create()
  {
    return MakePtr<FFTAnalyzerImpl>();
  }
}  // namespace Sound
