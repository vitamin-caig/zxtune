/**
* 
* @file
*
* @brief  Analyzer implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "analyzer.h"
//common includes
#include <make_ptr.h>
//library includes
#include <sound/chunk.h>
//std includes
#include <algorithm>
#include <array>
#include <complex>
#include <utility>

namespace Module
{
  class DevicesAnalyzer : public Analyzer
  {
  public:
    explicit DevicesAnalyzer(Devices::StateSource::Ptr delegate)
      : Delegate(std::move(delegate))
    {
    }

    std::vector<ChannelState> GetState() const override
    {
      const auto& in = Delegate->GetState();
      std::vector<ChannelState> out(in.size());
      std::transform(in.begin(), in.end(), out.begin(), &ConvertState);
      //required by compiler
      return std::move(out);
    }
  private:
    static ChannelState ConvertState(const Devices::ChannelState& in)
    {
      return {in.Band, in.Level.Raw()};
    }
  private:
    const Devices::StateSource::Ptr Delegate;
  };

  Analyzer::Ptr CreateAnalyzer(Devices::StateSource::Ptr state)
  {
    return MakePtr<DevicesAnalyzer>(state);
  }

  class FFTAnalyzer : public SoundAnalyzer
  {
  private:
    static const std::size_t WindowSizeLog = 10;
    static const std::size_t WindowSize = 1 << WindowSizeLog;
    static const std::size_t Points = WindowSize / 2;

  public:
    using Ptr = std::shared_ptr<FFTAnalyzer>;
    
    void AddSoundData(const Sound::Chunk& data) override
    {
      if (Active)
      {
        for (const auto& smp : data)
        {
          static_assert(Sound::Sample::MID == 0, "Incompatible sample type");
          const auto level = (smp.Left() + smp.Right()) / 2;
          Input[Cursor] = level;
          ++Cursor;
          Cursor %= WindowSize;
        }
      }
    }
    
    std::vector<ChannelState> GetState() const override
    {
      static const uint_t BANDS = 96;
      std::vector<ChannelState> result;
      result.reserve(BANDS);
      const auto& levels = FFT();
      ChannelState res;
      for (res.Band = 0; res.Band < BANDS; ++res.Band)
      {
        const auto begin = levels.begin() + res.Band * levels.size() / BANDS;
        const auto end = levels.begin() + (res.Band + 1) * levels.size() / BANDS;
        const auto maximum = *std::max_element(begin, end);
        if ((res.Level = std::min<uint_t>(100, maximum / 32)))
        {
          result.push_back(res);
        }
      }
      Active = true;
      return result;
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
          const float angle = 2. * 3.14159265358 * idx / WindowSize;
          Sinus[idx] = std::sin(angle);
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
      static uint_t Reverse(uint_t val)
      {
        return Instance().BitRev[val];
      }
      
      static Complex Polar(uint_t val)
      {
        const auto& sinus = Instance().Sinus;
        return Complex(sinus[(val + WindowSize / 4) % WindowSize], sinus[val]);
      }
    private:
      std::array<uint_t, WindowSize> BitRev;
      std::array<float, WindowSize> Sinus;
    };
  
    std::array<uint_t, Points> FFT() const
    {
      std::array<Complex, WindowSize> cplx;
      {
        for (std::size_t idx = 0; idx < WindowSize; ++idx)
        {
          cplx[idx] = Input[(Cursor + Lookup::Reverse(idx)) % WindowSize];
        }
      }
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
      
      std::array<uint_t, Points> result;
      for (std::size_t i = 0; i < result.size(); ++i)
      {
        result[i] = std::abs(cplx[i + 1]) / 256;
      }
      result.back() /= 2;
      return result;
    }
  private:
    mutable bool Active = false;
    std::array<int_t, WindowSize> Input;
    std::size_t Cursor = 0;
  };

  SoundAnalyzer::Ptr CreateSoundAnalyzer()
  {
    return MakePtr<FFTAnalyzer>();
  }
}
