/**
 *
 * @file
 *
 * @brief  FFT analyzer implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include <sound/impl/fft_analyzer.h>

#include <make_ptr.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <mutex>
#include <numeric>
#include <utility>
#include <vector>

namespace Sound
{
  namespace FFT
  {
    using Complex = std::complex<float>;

    template<std::size_t WindowSize>
    struct Core
    {
      static void Transform(const int_t* input, std::size_t cursor, Analyzer::LevelType* result, std::size_t limit)
      {
        auto data = ToComplex(input, cursor);
        DFFT(data);
        Decimate(data);
        Convert(data, result, limit);
      }

    private:
      static_assert(0 == (WindowSize & (WindowSize - 1)), "WindowSize should be power of 2");
      constexpr static const std::size_t Bits = Math::Log2(WindowSize - 1);
      constexpr static const auto PI = 3.14159265358f;

      static std::array<Complex, WindowSize> ToComplex(const int_t* input, std::size_t cursor)
      {
        static_assert(Sound::Sample::MID == 0, "Incompatible sample type");
        static const auto window = HannWindow();
        constexpr const auto INPUT_MAX = float(-Sound::Sample::MIN);
        std::array<Complex, WindowSize> result;
        for (uint_t idx = 0; idx < WindowSize; ++idx)
        {
          const auto val = float(input[(cursor + idx) % WindowSize]) / INPUT_MAX;
          result[idx] = Complex(window[idx] * val);
        }
        return result;
      }

      static std::array<Complex, WindowSize> HannWindow()
      {
        std::array<Complex, WindowSize> result;
        // https://en.wikipedia.org/wiki/Hann_function
        for (uint_t idx = 0; idx < WindowSize; ++idx)
        {
          const auto s = std::sin(PI * idx / (WindowSize - 1));
          result[idx] = s * s;
        }
        return result;
      }

      static void DFFT(std::array<Complex, WindowSize>& data)
      {
        constexpr const auto theta = PI / WindowSize;
        auto phi = std::polar(1.0f, -theta);
        for (std::size_t k = WindowSize / 2; k != 0; k >>= 1)
        {
          phi *= phi;
          Complex t = 1.0f;
          for (std::size_t l = 0; l < k; ++l)
          {
            for (auto a = l; a < WindowSize; a += k * 2)
            {
              const auto b = a + k;
              const auto diff = data[a] - data[b];
              data[a] += data[b];
              data[b] = diff * t;
            }
            t *= phi;
          }
        }
      }

      static void Decimate(std::array<Complex, WindowSize>& data)
      {
        for (std::size_t a = 0; a < WindowSize; ++a)
        {
          const auto b = BitRev(a);
          if (b > a)
          {
            std::swap(data[a], data[b]);
          }
        }
      }

      static uint32_t BitRev(uint32_t b)
      {
        b = ((b & 0xaaaaaaaa) >> 1) | ((b & 0x55555555) << 1);
        b = ((b & 0xcccccccc) >> 2) | ((b & 0x33333333) << 2);
        b = ((b & 0xf0f0f0f0) >> 4) | ((b & 0x0f0f0f0f) << 4);
        b = ((b & 0xff00ff00) >> 8) | ((b & 0x00ff00ff) << 8);
        b = (b >> 16) | (b << 16);
        return b >> (32 - Bits);
      }

      static void Convert(const std::array<Complex, WindowSize>& data, Analyzer::LevelType* result, std::size_t limit)
      {
        const std::size_t maxBands = WindowSize / 2 - 1;
        const std::size_t toFill = std::min(limit, maxBands);
        for (std::size_t i = 0; i < toFill; ++i)
        {
          const auto centiBells = To500cB(data[i + 1]);
          result[i] = Analyzer::LevelType(centiBells + 500, 500);
        }
        std::fill_n(result + toFill, limit - toFill, Analyzer::LevelType());
      }

      // scales to -50dB..0dB
      static int_t To500cB(Complex val)
      {
        const auto norm = std::norm(val / float(WindowSize / 4));
        const auto minimum = 1e-5f;
        return norm < minimum ? -500 : std::min(int_t(100 * std::log10(norm)), 0);
      }
    };
  }  // namespace FFT

  //! @brief %Sound analyzer interface
  class FFTAnalyzerImpl : public FFTAnalyzer
  {
  public:
    void GetSpectrum(LevelType* result, std::size_t limit) const override
    {
      const std::scoped_lock lock(Guard);
      if (Core)
      {
        Core(Input.data(), Cursor, result, limit);
      }
      else
      {
        Initialize(limit);
        std::fill_n(result, limit, LevelType());
      }
      Produced = 0;
    }

    void FeedSound(const Sample* samples, std::size_t count) override
    {
      const uint_t MAX_PRODUCED_DELTA = 10;
      if (Produced >= MAX_PRODUCED_DELTA)
      {
        return;
      }
      const std::scoped_lock lock(Guard);
      const auto windowSize = Input.size();
      if (!windowSize)
      {
        return;
      }
      if (count >= windowSize)
      {
        samples = samples + count - windowSize;
        count = windowSize;
      }
      for (auto *it = samples, *lim = samples + count; it != lim; ++it)
      {
        const auto level = (it->Left() + it->Right()) / 2;
        Input[Cursor] = level;
        if (++Cursor == windowSize)
        {
          Cursor = 0;
          ++Produced;
        }
      }
    }

  private:
    using CoreFunction = decltype(&FFT::Core<1>::Transform);

    void Initialize(std::size_t bars) const
    {
      const auto windowSize = 2 << Math::Log2(bars - 1);
      Core = CreateCore(windowSize);
      Input = std::vector<int_t>(windowSize);
      Cursor = 0;
    }

    static CoreFunction CreateCore(std::size_t windowSize)
    {
      switch (windowSize)
      {
      case 32:
        return &FFT::Core<32>::Transform;
      case 64:
        return &FFT::Core<64>::Transform;
      case 128:
        return &FFT::Core<128>::Transform;
      case 256:
        return &FFT::Core<256>::Transform;
      case 512:
        return &FFT::Core<512>::Transform;
      default:
        return nullptr;  // lets crash
      }
    }

  private:
    mutable std::mutex Guard;
    mutable CoreFunction Core = nullptr;
    mutable uint_t Produced = 0;
    mutable std::vector<int_t> Input;
    mutable std::size_t Cursor = 0;
  };

  FFTAnalyzer::Ptr FFTAnalyzer::Create()
  {
    return MakePtr<FFTAnalyzerImpl>();
  }
}  // namespace Sound
