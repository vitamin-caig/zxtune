/*
Abstract:
  TFM chip adapter

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//library includes
#include <devices/tfm.h>
//boost includes
#include <boost/make_shared.hpp>
#include <boost/mem_fn.hpp>

namespace Devices
{
namespace TFM
{
  class TurboChip : public Chip
  {
  public:
    TurboChip(ChipParameters::Ptr params, Sound::Receiver::Ptr target)
    {
      const std::pair<Sound::Receiver::Ptr, Sound::Receiver::Ptr> targets = Sound::CreateReceiversPair(target);
      Delegates[0] = Devices::FM::CreateChip(params, targets.first);
      Delegates[1] = Devices::FM::CreateChip(params, targets.second);
    }

    virtual void RenderData(const DataChunk& src)
    {
      Devices::FM::DataChunk chunk;
      chunk.TimeStamp = src.TimeStamp;
      for (uint_t idx = 0; idx != Delegates.size(); ++idx)
      {
        chunk.Data = src.Data[idx];
        Delegates[idx]->RenderData(chunk);
      }
    }

    virtual void Flush()
    {
      for (uint_t idx = 0; idx != Delegates.size(); ++idx)
      {
        Delegates[idx]->Flush();
      }
    }

    virtual void Reset()
    {
      std::for_each(Delegates.begin(), Delegates.end(), boost::mem_fn(&Devices::FM::Chip::Reset));
    }

    virtual void GetState(Devices::TFM::ChannelsState& state) const
    {
      for (uint_t idx = 0; idx != Delegates.size(); ++idx)
      {
        Devices::FM::ChannelsState subState;
        Delegates[idx]->GetState(subState);
        const std::size_t inChans = subState.size();
        for (uint_t chan = 0; chan != inChans; ++chan)
        {
          Devices::FM::ChanState& dst = state[idx * inChans + chan];
          dst = subState[chan];
          dst.Name = Char(dst.Name + idx * inChans);
          dst.Band += idx * inChans;
        }
      }
    }
  private:
    boost::array<Devices::FM::Chip::Ptr, CHIPS> Delegates;
  };
}//TFM
}//Devices

namespace Devices
{
  namespace TFM
  {
    Chip::Ptr CreateChip(ChipParameters::Ptr params, Sound::Receiver::Ptr target)
    {
      return boost::make_shared<TurboChip>(params, target);
    }
  }
}
