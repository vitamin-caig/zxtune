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
#include <xrange.h>
// library includes
#include <sound/resampler.h>

extern "C"
{
// 3rdparty includes
#include <3rdparty/lazyusf2/usf/resampler.h>
}

namespace Sound
{
  class CubicCore
  {
  public:
    CubicCore(uint_t freqIn, uint_t freqOut)
      : FreqIn(freqIn)
      , FreqOut(freqOut)
      , Delegate(::resampler_create(), &::resampler_delete)
    {
      ::resampler_set_rate(Delegate.get(), double(freqIn) / freqOut);
    }

    Chunk Apply(Chunk data)
    {
      Chunk result;
      result.reserve(data.size() * FreqOut / FreqIn + 1);
      const Sample* in = data.data();
      for (std::size_t rest = data.size(); rest != 0;)
      {
        const auto fed = Feed(in, rest);
        const auto drained = Drain(&result);
        in += fed;
        rest -= fed;
        if (!fed && !drained)
        {
          break;  // for any
        }
      }
      return result;
    }

  private:
    std::size_t Feed(const Sample* in, std::size_t limit)
    {
      const auto avail = ::resampler_get_free_count(Delegate.get());
      if (avail > 0)
      {
        const auto toDo = std::min(limit, std::size_t(avail));
        for (const auto* smp : xrange(in, in + toDo))
        {
          ::resampler_write_sample(Delegate.get(), smp->Left(), smp->Right());
        }
        return toDo;
      }
      return 0;
    }

    std::size_t Drain(Chunk* output)
    {
      Sample::Type left = 0;
      Sample::Type right = 0;
      const auto avail = ::resampler_get_sample_count(Delegate.get());
      for (int i = 0; i < avail; ++i)
      {
        ::resampler_get_sample(Delegate.get(), &left, &right);
        ::resampler_remove_sample(Delegate.get());
        output->emplace_back(left, right);
      }
      return std::size_t(avail);
    }

  private:
    const uint_t FreqIn;
    const uint_t FreqOut;
    const std::shared_ptr<void> Delegate;
  };

  class IdentityCore
  {
  public:
    IdentityCore(uint_t freqIn, uint_t freqOut)
    {
      Require(freqIn == freqOut);
    }

    static Chunk Apply(Chunk data)
    {
      return Chunk(std::move(data));
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
    else
    {
      return MakePtr<Resampler<CubicCore>>(inFreq, outFreq);
    }
  }
}  // namespace Sound
