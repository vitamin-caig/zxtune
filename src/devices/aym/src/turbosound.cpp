/**
* 
* @file
*
* @brief  TurboSound chip implementation
*
* @author vitamin.caig@gmail.com
*
**/

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
      const Sample s0 = Chip0.GetLevels();
      const Sample s1 = Chip1.GetLevels();
      return Sound::Sample::FastAdd(s0, s1);
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

  class HalfLevelMixer : public MixerType
  {
  public:
    explicit HalfLevelMixer(MixerType::Ptr delegate)
      : Delegate(delegate)
      , DelegateRef(*Delegate)
    {
    }

    virtual Sound::Sample ApplyData(const MixerType::InDataType& in) const
    {
      const Sound::Sample out = DelegateRef.ApplyData(in);
      return Sound::Sample(out.Left() / 2, out.Right() / 2);
    }
  private:
    const MixerType::Ptr Delegate;
    const MixerType& DelegateRef;
  };

  Chip::Ptr CreateChip(ChipParameters::Ptr params, MixerType::Ptr mixer, Sound::Receiver::Ptr target)
  {
    const MixerType::Ptr halfMixer = boost::make_shared<HalfLevelMixer>(mixer);
    return boost::make_shared<AYM::SoundChip<Traits> >(params, halfMixer, target);
  }
}
}
