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
#include <error.h>
#include <make_ptr.h>
//library includes
#include <l10n/api.h>
#include <math/numeric.h>
#include <math/fixedpoint.h>
#include <sound/gainer.h>
//boost includes
#include <boost/integer/static_log2.hpp>

#define FILE_TAG F5996093

namespace Sound
{
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("sound");

  class GainCore
  {
  public:
    GainCore()
      : Level()
      , Step()
    {
    }

    Sample Apply(Sample in) const
    {
      static const Coeff ONE(1);
      static const Coeff ZERO(0);
      if (Level == ONE)
      {
        return in;
      }
      else if (Level == ZERO)
      {
        return Sample();
      }
      return Sample((Level * in.Left()).Round(), (Level * in.Right()).Round());
    }

    void SetGain(Gain::Type in)
    {
      static const Gain::Type MIN(0, Gain::Type::PRECISION);
      static const Gain::Type MAX(Gain::Type::PRECISION, Gain::Type::PRECISION);
      if (in < MIN || in > MAX)
      {
        throw Error(THIS_LINE, translate("Failed to set gain value: out of range."));
      }
      Level = Coeff(in);
    }

    void SetFading(Gain::Type delta, uint_t step)
    {
      Step = Coeff(delta) / step;
    }

    void ApplyStep()
    {
      static const Coeff ONE(1);
      static const Coeff ZERO(0);
      if (Step != ZERO)
      {
        Level += Step;
        if (Level > ONE || Level < ZERO)
        {
          Level = Step < ZERO ? ZERO : ONE;
          Step = ZERO;
        }
      }
    }
  private:
    typedef Math::FixedPoint<int64_t, int64_t(1) << 31> Coeff;
    static_assert(8 * sizeof(Coeff) >= boost::static_log2<Coeff::PRECISION>::value + Sample::BITS, "Not enough bits");
    Coeff Level;
    Coeff Step;
  };

  class FixedPointGainer : public FadeGainer
  {
  public:
    FixedPointGainer()
      : Delegate(Receiver::CreateStub())
      , Core()
    {
    }

    void ApplyData(Chunk in) override
    {
      for (auto& val : in)
      {
        val = Core.Apply(val);
      }
      Core.ApplyStep();
      return Delegate->ApplyData(std::move(in));
    }

    void Flush() override
    {
      Delegate->Flush();
    }

    void SetTarget(Receiver::Ptr delegate) override
    {
      Delegate = delegate ? delegate : Receiver::CreateStub();
    }

    void SetGain(Gain::Type gain) override
    {
      Core.SetGain(gain);
    }

    void SetFading(Gain::Type delta, uint_t step) override
    {
      Core.SetFading(delta, step);
    }
  private:
    Receiver::Ptr Delegate;
    GainCore Core;
  };
}

namespace Sound
{
  FadeGainer::Ptr CreateFadeGainer()
  {
    return MakePtr<FixedPointGainer>();
  }
}
