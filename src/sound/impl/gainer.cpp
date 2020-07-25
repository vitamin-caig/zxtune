/**
*
* @file
*
* @brief  Gain control implementation
*
* @author vitamin.caig@gmail.com
*
**/

//common includes
#include <make_ptr.h>
//library includes
#include <math/numeric.h>
#include <math/fixedpoint.h>
#include <sound/gainer.h>
//boost includes
#include <boost/integer/static_log2.hpp>

namespace Sound
{
  const auto USED_GAIN_BITS = boost::static_log2<Gain::Type::PRECISION>::value + Sample::BITS;
  const auto AVAIL_GAIN_BITS = 8 * sizeof(Gain::Type::ValueType);
    
  static_assert(USED_GAIN_BITS < AVAIL_GAIN_BITS, "Not enough bits");
  const Gain::Type MAX_LEVEL(1 << (AVAIL_GAIN_BITS - USED_GAIN_BITS));

  class GainCore
  {
  public:
    GainCore()
      : Level(1)
    {
    }

    Sample Apply(Sample in) const
    {
      return Sample(Clamp((Level * in.Left()).Round()), Clamp((Level * in.Right()).Round()));
    }

    bool SetGain(Gain::Type in)
    {
      Level = std::min(in, MAX_LEVEL);
      return Level != Gain::Type(1);
    }

  private:
    static Sample::Type Clamp(Gain::Type::ValueType in)
    {
      return Math::Clamp<Gain::Type::ValueType>(in, Sample::MIN, Sample::MAX);
    }

  private:
    Gain::Type Level;
  };

  class FixedPointGainer : public Receiver
  {
  public:
    FixedPointGainer(GainSource::Ptr gain, Receiver::Ptr delegate)
      : Gain(std::move(gain))
      , Delegate(std::move(delegate))
      , Core()
    {
    }

    void ApplyData(Chunk in) override
    {
      const auto gain = Gain->Get();
      if (Core.SetGain(gain))
      {
        for (auto& val : in)
        {
          val = Core.Apply(val);
        }
      }
      return Delegate->ApplyData(std::move(in));
    }

    void Flush() override
    {
      Delegate->Flush();
    }

  private:
    const GainSource::Ptr Gain;
    const Receiver::Ptr Delegate;
    GainCore Core;
  };

  Receiver::Ptr CreateGainer(GainSource::Ptr gain, Receiver::Ptr delegate)
  {
    return MakePtr<FixedPointGainer>(std::move(gain), std::move(delegate));
  }
}
