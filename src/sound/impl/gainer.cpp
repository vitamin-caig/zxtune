/*
Abstract:
  Gain control implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//common includes
#include <error.h>
//library includes
#include <l10n/api.h>
#include <math/numeric.h>
#include <math/fixedpoint.h>
#include <sound/gainer.h>
//boost includes
#include <boost/make_shared.hpp>
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

    int_t Apply(int_t in)
    {
      static const Coeff ONE(1);
      static const Coeff ZERO(0);
      if (Level == ONE)
      {
        return in;
      }
      else if (Level == ZERO)
      {
        return 0;
      }
      return (Level * in).Integer();
    }

    void SetGain(double in)
    {
      if (!Math::InRange<double>(in, 0.0, 1.0))
      {
        throw Error(THIS_LINE, translate("Failed to set gain value: out of range."));
      }
      Level = in;
    }

    void SetFading(double delta, uint_t step)
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
        if (Level > ONE)
        {
          Level = Step < ZERO ? ZERO : ONE;
          Step = ZERO;
        }
      }
    }
  private:
    typedef Math::FixedPoint<int64_t, 1 << 31> Coeff;
    BOOST_STATIC_ASSERT(8 * sizeof(Coeff) >= boost::static_log2<Coeff::PRECISION>::value + Sample::BITS);
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

    virtual void ApplyData(const Sample& in)
    {
      const Sample out(Core.Apply(in.Left()), Core.Apply(in.Right()));
      Core.ApplyStep();
      return Delegate->ApplyData(out);
    }

    virtual void Flush()
    {
      Delegate->Flush();
    }

    virtual void SetTarget(Receiver::Ptr delegate)
    {
      Delegate = delegate ? delegate : Receiver::CreateStub();
    }

    virtual void SetGain(double gain)
    {
      Core.SetGain(gain);
    }

    virtual void SetFading(double delta, uint_t step)
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
    return boost::make_shared<FixedPointGainer>();
  }
}
