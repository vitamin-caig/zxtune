/**
* 
* @file
*
* @brief  TurboFM chips implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "chip.h"
//common includes
#include <make_ptr.h>
//library includes
#include <devices/tfm.h>
//std includes
#include <functional>

namespace Devices
{
namespace TFM
{
  class ChipAdapter
  {
  public:
    void SetParams(uint64_t clock, uint_t sndFreq)
    {
      if (Helper.SetNewParams(clock, sndFreq))
      {
        Chips[0] = Helper.CreateChip();
        Chips[1] = Helper.CreateChip();
      }
    }

    void Reset()
    {
      if (Chips[0])
      {
        ::YM2203ResetChip(Chips[0].get());
        ::YM2203ResetChip(Chips[1].get());
      }
    }

    void WriteRegisters(const Devices::TFM::Registers& regs)
    {
      for (Registers::const_iterator it = regs.begin(), lim = regs.end(); it != lim; ++it)
      {
        const Register reg = *it;
        ::YM2203WriteRegs(Chips[reg.Chip()].get(), reg.Index(), reg.Value());
      }
    }

    void RenderSamples(uint_t count, Sound::ChunkBuilder& tgt)
    {
      Sound::Sample* const out = tgt.Allocate(count);
      FM::Details::YM2203SampleType* const outRaw = safe_ptr_cast<FM::Details::YM2203SampleType*>(out);
      ::YM2203UpdateOne(Chips[0].get(), outRaw, count);
      ::YM2203UpdateOne(Chips[1].get(), outRaw, count);
      std::transform(outRaw, outRaw + count, outRaw, std::bind2nd(std::divides<FM::Details::YM2203SampleType>(), 2));
      Helper.ConvertSamples(outRaw, outRaw + count, out);
    }

    void GetState(MultiChannelState& state) const
    {
      MultiChannelState res;
      res.reserve(VOICES);
      std::array<uint_t, FM::VOICES> attenuations;
      std::array<uint_t, FM::VOICES> periods;
      ::YM2203GetState(Chips[0].get(), &attenuations[0], &periods[0]);
      Helper.ConvertState(attenuations.data(), periods.data(), res);
      ::YM2203GetState(Chips[1].get(), &attenuations[0], &periods[0]);
      Helper.ConvertState(attenuations.data(), periods.data(), res);
      state.swap(res);
    }
  private:
    FM::Details::ChipAdapterHelper Helper;
    std::array<FM::Details::ChipPtr, TFM::CHIPS> Chips;
  };

  struct Traits
  {
    typedef Chip BaseClass;
    typedef DataChunk DataChunkType;
    typedef Stamp StampType;
    typedef ChipAdapter AdapterType;
  };

  typedef FM::Details::BaseChip<Traits> TFMChip;

  Chip::Ptr CreateChip(ChipParameters::Ptr params, Sound::Receiver::Ptr target)
  {
    return MakePtr<TFMChip>(params, target);
  }
}
}
