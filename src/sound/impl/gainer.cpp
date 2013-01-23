/*
Abstract:
  Gain control implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "internal_types.h"
//common includes
#include <error.h>
//library includes
#include <l10n/api.h>
#include <math/numeric.h>
#include <sound/gainer.h>
//boost includes
#include <boost/make_shared.hpp>
#include <boost/integer/static_log2.hpp>

#define FILE_TAG F5996093

namespace
{
  using namespace ZXTune::Sound;

  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("sound");

  class GainCore
  {
    typedef uint64_t LevelType;
    typedef int64_t StepType;
    static const LevelType DIVIDER = LevelType(1) << 32;
    BOOST_STATIC_ASSERT(8 * sizeof(LevelType) >= boost::static_log2<DIVIDER>::value + 8 * sizeof(Sample));
  public:
    GainCore()
      : Level()
      , Step()
    {
    }

    Sample Apply(Sample in)
    {
      if (DIVIDER == Level)
      {
        return in;
      }
      else if (0 == Level)
      {
        return SAMPLE_MID;
      }
      const int_t normalized = int_t(in) - SAMPLE_MID;
      const int_t scaled = Level * normalized / DIVIDER;
      return static_cast<Sample>(scaled + SAMPLE_MID);
    }

    void SetGain(Gain in)
    {
      if (!Math::InRange<Gain>(in, 0.0f, 1.0f))
      {
        throw Error(THIS_LINE, translate("Failed to set gain value: out of range."));
      }
      Level = static_cast<LevelType>(in * DIVIDER);
    }

    void SetFading(Gain delta, uint_t step)
    {
      Step = static_cast<StepType>(delta * DIVIDER / step);
    }

    void ApplyStep()
    {
      if (Step)
      {
        Level += Step;
        if (Level > DIVIDER)
        {
          Level = Step >= 0 ? DIVIDER : 0;
          Step = 0;
        }
      }
    }
  private:
    LevelType Level;
    StepType Step;
  };

  class FixedPointGainer : public FadeGainer
  {
  public:
    FixedPointGainer()
      : Delegate(Receiver::CreateStub())
      , Core()
    {
    }

    virtual void ApplyData(const MultiSample& data)
    {
      MultiSample result;
      for (uint_t chan = 0; chan != OUTPUT_CHANNELS; ++chan)
      {
        result[chan] = Core.Apply(data[chan]);
      }
      Core.ApplyStep();
      return Delegate->ApplyData(result);
    }

    virtual void Flush()
    {
      Delegate->Flush();
    }

    virtual void SetTarget(Receiver::Ptr delegate)
    {
      Delegate = delegate ? delegate : Receiver::CreateStub();
    }

    virtual void SetGain(Gain gain)
    {
      Core.SetGain(gain);
    }

    virtual void SetFading(Gain delta, uint_t step)
    {
      Core.SetFading(delta, step);
    }
  private:
    Receiver::Ptr Delegate;
    GainCore Core;
  };
}

namespace ZXTune
{
  namespace Sound
  {
    FadeGainer::Ptr CreateFadeGainer()
    {
      return boost::make_shared<FixedPointGainer>();
    }
  }
}
