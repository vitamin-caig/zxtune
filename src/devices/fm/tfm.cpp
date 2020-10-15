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
      for (const auto& reg : regs)
      {
        ::YM2203WriteRegs(Chips.at(reg.Chip()).get(), reg.Index(), reg.Value());
      }
    }

    Sound::Chunk RenderSamples(uint_t count)
    {
      Sound::Chunk result(count);
      auto* const outRaw = safe_ptr_cast<FM::Details::YM2203SampleType*>(result.data());
      ::YM2203UpdateOne(Chips[0].get(), outRaw, count);
      ::YM2203UpdateOne(Chips[1].get(), outRaw, count);
      std::transform(outRaw, outRaw + count, outRaw, [](FM::Details::YM2203SampleType s) {return s / 2;});
      Helper.ConvertSamples(outRaw, outRaw + count, result.data());
      return result;
    }

    DeviceState GetState() const
    {
      DeviceState res;
      std::array<uint_t, FM::VOICES> attenuations;
      std::array<uint_t, FM::VOICES> periods;
      ::YM2203GetState(Chips[0].get(), attenuations.data(), periods.data());
      Helper.ConvertState(attenuations.data(), periods.data(), res);
      ::YM2203GetState(Chips[1].get(), attenuations.data(), periods.data());
      Helper.ConvertState(attenuations.data(), periods.data(), res);
      return res;
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

  Chip::Ptr CreateChip(ChipParameters::Ptr params)
  {
    return MakePtr<TFMChip>(std::move(params));
  }
}
}
