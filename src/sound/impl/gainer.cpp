/**
 *
 * @file
 *
 * @brief  Gain control implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "sound/gainer.h"

#include "math/bitops.h"
#include "math/fixedpoint.h"
#include "math/numeric.h"
#include "sound/chunk.h"

#include "make_ptr.h"

#include <algorithm>

namespace Sound
{
  constexpr const auto USED_GAIN_BITS = Math::Log2(Gain::Type::PRECISION) + Sample::BITS;
  constexpr const auto AVAIL_GAIN_BITS = 8 * sizeof(Gain::Type::ValueType);

  static_assert(USED_GAIN_BITS < AVAIL_GAIN_BITS, "Not enough bits");
  const Gain::Type MAX_LEVEL(1 << (AVAIL_GAIN_BITS - USED_GAIN_BITS));

  class GainCore
  {
  public:
    GainCore()
      : Level(1)
    {}

    Sample Apply(Sample in) const
    {
      return {Clamp((Level * in.Left()).Round()), Clamp((Level * in.Right()).Round())};
    }

    bool SetGain(Gain::Type in)
    {
      Level = std::min(in, MAX_LEVEL);
      return !IsIdentity();
    }

    bool IsIdentity() const
    {
      return Level == Gain::Type(1);
    }

  private:
    static Sample::Type Clamp(Gain::Type::ValueType in)
    {
      return Math::Clamp<Gain::Type::ValueType>(in, Sample::MIN, Sample::MAX);
    }

  private:
    Gain::Type Level;
  };

  class FixedPointGainer : public Gainer
  {
  public:
    void SetGain(Gain::Type gain) override
    {
      Core.SetGain(gain);
    }

    Chunk Apply(Chunk in) override
    {
      if (!Core.IsIdentity())
      {
        for (auto& val : in)
        {
          val = Core.Apply(val);
        }
      }
      return in;
    }

  private:
    GainCore Core;
  };

  Gainer::Ptr CreateGainer()
  {
    return MakePtr<FixedPointGainer>();
  }
}  // namespace Sound
