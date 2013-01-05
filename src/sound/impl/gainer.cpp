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
#include <boost/mpl/if.hpp>

#define FILE_TAG F5996093

namespace
{
  using namespace ZXTune::Sound;

  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("sound");

  class GainCore
  {
    static const uint_t MIN_LEVEL_BITS =
      2 * boost::static_log2<FIXED_POINT_PRECISION>::value +  //divider
      8 * sizeof(Sample);                                     //sample
    typedef typename boost::mpl::if_c<
      MIN_LEVEL_BITS <= 8 * sizeof(uint_t),
      uint_t,
      uint64_t
    >::type LevelType;
    typedef LevelType StepType;
    static const LevelType DIVIDER = FIXED_POINT_PRECISION * FIXED_POINT_PRECISION;

    BOOST_STATIC_ASSERT(8 * sizeof(LevelType) >= MIN_LEVEL_BITS);
  public:
    GainCore()
      : Level()
      , Step()
    {
    }

    Sample Apply(Sample in)
    {
      const int_t normalized = int_t(in) - SAMPLE_MID;
      const int_t scaled = Level * normalized / DIVIDER;
      if (Step)
      {
        Level += Step;
        if (Level > DIVIDER)
        {
          Level = Step > 0 ? DIVIDER : 0;
          Step = 0;
        }
      }
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
  private:
    LevelType Level;
    StepType Step;
  };

  typedef boost::array<GainCore, OUTPUT_CHANNELS> MultiGainCore;

  class FixedPointGainer : public FadeGainer
  {
  public:
    FixedPointGainer()
      : Delegate(Receiver::CreateStub())
      , Cores()
    {
    }

    virtual void ApplyData(const MultiSample& data)
    {
      MultiSample result;
      for (uint_t chan = 0; chan != OUTPUT_CHANNELS; ++chan)
      {
        result[chan] = Cores[chan].Apply(data[chan]);
      }
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
      MultiGainCore tmp(Cores);
      for (uint_t chan = 0; chan != OUTPUT_CHANNELS; ++chan)
      {
        tmp[chan].SetGain(gain);
      }
      Cores.swap(tmp);
    }

    virtual void SetFading(Gain delta, uint_t step)
    {
      MultiGainCore tmp(Cores);
      for (uint_t chan = 0; chan != OUTPUT_CHANNELS; ++chan)
      {
        tmp[chan].SetFading(delta, step);
      }
      Cores.swap(tmp);
    }
  private:
    Receiver::Ptr Delegate;
    MultiGainCore Cores;
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
