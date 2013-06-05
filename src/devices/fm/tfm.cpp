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
  //TODO: reduce C&P
  class DoubleMixer
  {
  public:
    typedef boost::shared_ptr<DoubleMixer> Ptr;

    DoubleMixer()
    {
    }

    void Store(Sound::Chunk::Ptr in)
    {
      Buffer = in;
    }

    Sound::Chunk::Ptr Mix(Sound::Chunk::Ptr in)
    {
      assert(in->size() == Buffer->size());
      std::transform(Buffer->begin(), Buffer->end(), in->begin(), Buffer->begin(), &MixAll);
      return Buffer;
    }
  private:
    static inline Sound::Sample::Type MixChannel(Sound::Sample::Type lh, Sound::Sample::Type rh)
    {
      return (int_t(lh) + rh) / 2;
    }

    static inline Sound::Sample MixAll(Sound::Sample lh, Sound::Sample rh)
    {
      return Sound::Sample(MixChannel(lh.Left(), rh.Left()), MixChannel(lh.Right(), rh.Right()));
    }
  private:
    Sound::Chunk::Ptr Buffer;
  };

  class FirstReceiver : public Sound::Receiver
  {
  public:
    explicit FirstReceiver(DoubleMixer::Ptr mixer)
      : Mixer(mixer)
    {
    }

    virtual void ApplyData(const Sound::Chunk::Ptr& data)
    {
      Mixer->Store(data);
    }

    virtual void Flush()
    {
    }
  private:
    const DoubleMixer::Ptr Mixer;
  };

  class SecondReceiver : public Sound::Receiver
  {
  public:
    SecondReceiver(DoubleMixer::Ptr mixer, Sound::Receiver::Ptr delegate)
      : Mixer(mixer)
      , Delegate(delegate)
    {
    }

    virtual void ApplyData(const Sound::Chunk::Ptr& data)
    {
      Delegate->ApplyData(Mixer->Mix(data));
    }

    virtual void Flush()
    {
      Delegate->Flush();
    }
  private:
    const DoubleMixer::Ptr Mixer;
    const Sound::Receiver::Ptr Delegate;
  };

  class TurboChip : public Chip
  {
  public:
    TurboChip(ChipParameters::Ptr params, Sound::Receiver::Ptr target)
    {
      const DoubleMixer::Ptr mixer = boost::make_shared<DoubleMixer>();
      Delegates[0] = Devices::FM::CreateChip(params, boost::make_shared<FirstReceiver>(mixer));
      Delegates[1] = Devices::FM::CreateChip(params, boost::make_shared<SecondReceiver>(mixer, target));
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
