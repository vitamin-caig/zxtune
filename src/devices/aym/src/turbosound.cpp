/*
Abstract:
  TurboSound chip adapter

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "psg.h"
#include "soundchip.h"
//library includes
#include <devices/turbosound.h>
//boost includes
#include <boost/make_shared.hpp>

namespace Devices
{
namespace TurboSound
{
  class PSG
  {
  public:
    explicit PSG(const AYM::MultiVolumeTable& table)
      : Chip0(table)
      , Chip1(table)
    {
    }

    void SetDutyCycle(uint_t value, uint_t mask)
    {
      Chip0.SetDutyCycle(value, mask);
      Chip1.SetDutyCycle(value, mask);
    }

    void Reset()
    {
      Chip0.Reset();
      Chip1.Reset();
    }

    void SetNewData(const Registers& data)
    {
      Chip0.SetNewData(data[0]);
      Chip1.SetNewData(data[1]);
    }

    void Tick(uint_t ticks)
    {
      Chip0.Tick(ticks);
      Chip1.Tick(ticks);
    }

    Sound::Sample GetLevels() const
    {
      using namespace Sound;
      const Sample s0 = Chip0.GetLevelsNoBeeper();
      const Sample s1 = Chip1.GetLevelsNoBeeper();
      return AYM::Average(s0, s1);
    }

    void GetState(MultiChannelState& state) const
    {
      Chip0.GetState(state);
      Chip1.GetState(state);
    }
  private:
    AYM::PSG Chip0;
    AYM::PSG Chip1;
  };

  struct Traits
  {
    typedef DataChunk DataChunkType;
    typedef PSG PSGType;
    typedef Chip ChipBaseType;
    static const uint_t VOICES = TurboSound::VOICES;
  };

  Chip::Ptr CreateChip(ChipParameters::Ptr params, MixerType::Ptr mixer, Sound::Receiver::Ptr target)
  {
    return boost::make_shared<AYM::SoundChip<Traits> >(params, mixer, target);
  }
}
}
