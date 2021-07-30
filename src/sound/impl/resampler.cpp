/**
 *
 * @file
 *
 * @brief  Resampler implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// common includes
#include <contract.h>
#include <make_ptr.h>
// library includes
#include <math/fixedpoint.h>
#include <sound/resampler.h>

#include <utility>

namespace Sound
{
  typedef Math::FixedPoint<int_t, 256> FixedStep;

  inline Sample::WideType Interpolate(Sample::WideType prev, FixedStep ratio, Sample::WideType next)
  {
    const Sample::WideType delta = next - prev;
    return prev + (ratio * delta).Integer();
  }

  inline Sample Interpolate(Sample prev, FixedStep ratio, Sample next)
  {
    if (0 == ratio.Raw())
    {
      return prev;
    }
    else
    {
      return Sample(Sound::Interpolate(prev.Left(), ratio, next.Left()),
                    Sound::Interpolate(prev.Right(), ratio, next.Right()));
    }
  }

  class UpsampleCore
  {
  public:
    UpsampleCore(uint_t freqIn, uint_t freqOut)
      : Step(freqIn, freqOut)
    {
      Require(freqIn < freqOut);
    }

    Chunk Apply(const Chunk& data)
    {
      if (data.empty())
      {
        return {};
      }
      auto it = data.begin(), lim = data.end();
      if (0 == Position.Raw())
      {
        Prev = *it;
        if (++it == lim)
        {
          // one sample - nothing to upscale
          return {};
        }
      }
      Chunk result;
      result.reserve(1 + (FixedStep(data.size()) / Step).Round());
      Sample next = *it;
      for (;; Position += Step)
      {
        if (Position.Raw() >= Position.PRECISION)
        {
          Position -= 1;
          Prev = next;
          if (++it == lim)
          {
            break;
          }
          next = *it;
        }
        result.push_back(Interpolate(Prev, Position, next));
      }
      return result;
    }

  private:
    const FixedStep Step;
    FixedStep Position;
    Sample Prev;
  };

  class DownsampleCore
  {
  public:
    DownsampleCore(uint_t freqIn, uint_t freqOut)
      : Step(freqOut, freqIn)
    {
      Require(freqIn > freqOut);
    }

    Chunk Apply(const Chunk& data)
    {
      if (data.empty())
      {
        return {};
      }
      auto it = data.begin(), lim = data.end();
      if (0 == Position.Raw())
      {
        Prev = *it;
        ++it;
      }
      Chunk result;
      result.reserve(1 + (Step * data.size()).Round());
      for (; it != lim; Position += Step, ++it)
      {
        const Sample next = *it;
        if (Position.Raw() >= Position.PRECISION)
        {
          Position -= 1;
          result.push_back(Interpolate(Prev, Position, next));
        }
        Prev = next;
      }
      return result;
    }

  private:
    const FixedStep Step;
    FixedStep Position;
    Sample Prev;
  };

  class IdentityCore
  {
  public:
    IdentityCore(uint_t freqIn, uint_t freqOut)
    {
      Require(freqIn == freqOut);
    }

    Chunk Apply(Chunk data)
    {
      return std::move(data);
    }
  };

  template<class CoreType>
  class Resampler : public Converter
  {
  public:
    Resampler(uint_t freqIn, uint_t freqOut)
      : Core(freqIn, freqOut)
    {}

    Chunk Apply(Chunk in) override
    {
      return Core.Apply(std::move(in));
    }

  private:
    CoreType Core;
  };

  Converter::Ptr CreateResampler(uint_t inFreq, uint_t outFreq)
  {
    if (inFreq == outFreq)
    {
      return MakePtr<Resampler<IdentityCore>>(inFreq, outFreq);
    }
    else if (inFreq < outFreq)
    {
      return MakePtr<Resampler<UpsampleCore>>(inFreq, outFreq);
    }
    else
    {
      return MakePtr<Resampler<DownsampleCore>>(inFreq, outFreq);
    }
  }
}  // namespace Sound
