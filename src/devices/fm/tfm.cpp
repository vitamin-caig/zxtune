/*
Abstract:
  TFM chips implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "chip.h"
//library includes
#include <devices/tfm.h>
//boost includes
#include <boost/make_shared.hpp>

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
        Chip1 = Helper.CreateChip();
        Chip2 = Helper.CreateChip();
      }
    }

    void Reset()
    {
      if (Chip1)
      {
        ::YM2203ResetChip(Chip1.get());
        ::YM2203ResetChip(Chip2.get());
      }
    }

    void WriteRegisters(const Devices::TFM::DataChunk& chunk)
    {
      assert(Chip1);
      Helper.WriteRegisters(chunk.Data[0].begin(), chunk.Data[0].end(), Chip1.get());
      Helper.WriteRegisters(chunk.Data[1].begin(), chunk.Data[1].end(), Chip2.get());
    }

    void RenderSamples(uint_t count, Sound::ChunkBuilder& tgt)
    {
      assert(Chip1);
      Sound::Sample* const out = tgt.Allocate(count);
      FM::Details::YM2203SampleType* const outRaw = safe_ptr_cast<FM::Details::YM2203SampleType*>(out);
      ::YM2203UpdateOne(Chip1.get(), outRaw, count);
      ::YM2203UpdateOne(Chip2.get(), outRaw, count);
      Helper.ConvertSamples(outRaw, outRaw + count, out);
    }

    ChannelsState GetState() const
    {
      assert(Chip1);
      ChannelsState res(VOICES);
      boost::array<uint_t, FM::VOICES> attenuations;
      boost::array<uint_t, FM::VOICES> periods;
      ::YM2203GetState(Chip1.get(), &attenuations[0], &periods[0]);
      Helper.ConvertState(attenuations.begin(), periods.begin(), &res[0], 0u);
      ::YM2203GetState(Chip2.get(), &attenuations[0], &periods[0]);
      Helper.ConvertState(attenuations.begin(), periods.begin(), &res[FM::VOICES], FM::VOICES);
      return res;
    }
  private:
    FM::Details::ChipAdapterHelper Helper;
    FM::Details::ChipPtr Chip1;
    FM::Details::ChipPtr Chip2;
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
    return boost::make_shared<TFMChip>(params, target);
  }
}
}
