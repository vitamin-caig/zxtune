/*
Abstract:
  TurboSound chip adapter

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//library includes
#include <devices/turbosound.h>
//boost includes
#include <boost/make_shared.hpp>
#include <boost/mem_fn.hpp>

namespace Devices
{
namespace TurboSound
{
  //TODO: reduce C&P
  class DoubleMixer
  {
  public:
    typedef boost::shared_ptr<DoubleMixer> Ptr;

    DoubleMixer()
      : InCursor()
      , OutCursor()
    {
    }

    void Store(Sound::Sample in)
    {
      Buffer[InCursor++] = in;
    }

    Sound::Sample Mix(Sound::Sample in)
    {
      const Sound::Sample mix = Buffer[OutCursor++];
      return Sound::Sample(Mix(in.Left(), mix.Left()), Mix(in.Right(), mix.Right()));
    }
  private:
    static inline Sound::Sample::Type Mix(Sound::Sample::Type lh, Sound::Sample::Type rh)
    {
      return (int_t(lh) + rh) / 2;
    }
  private:
    boost::array<Sound::Sample, 65536> Buffer;
    uint16_t InCursor;
    uint16_t OutCursor;
  };

  class FirstReceiver : public Sound::Receiver
  {
  public:
    explicit FirstReceiver(DoubleMixer::Ptr mixer)
      : Mixer(mixer)
    {
    }

    virtual void ApplyData(const Sound::Sample& data)
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

    virtual void ApplyData(const Sound::Sample& data)
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
      Delegates[0] = AYM::CreateChip(params, boost::make_shared<FirstReceiver>(mixer));
      Delegates[1] = AYM::CreateChip(params, boost::make_shared<SecondReceiver>(mixer, target));
    }

    virtual void RenderData(const DataChunk& src)
    {
      AYM::DataChunk chunk;
      chunk.TimeStamp = src.TimeStamp;
      chunk.Mask = src.Mask0;
      chunk.Data = src.Data0;
      Delegates[0]->RenderData(chunk);
      chunk.Mask = src.Mask1;
      chunk.Data = src.Data1;
      Delegates[1]->RenderData(chunk);
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
      std::for_each(Delegates.begin(), Delegates.end(), boost::mem_fn(&AYM::Chip::Reset));
    }

    virtual void GetState(ChannelsState& state) const
    {
      for (uint_t idx = 0; idx != Delegates.size(); ++idx)
      {
        AYM::ChannelsState subState;
        Delegates[idx]->GetState(subState);
        const std::size_t inChans = subState.size();
        for (uint_t chan = 0; chan != inChans; ++chan)
        {
          AYM::ChanState& dst = state[idx * inChans + chan];
          dst = subState[chan];
          dst.Name = Char(dst.Name + idx * inChans);
          dst.Band += idx * inChans;
        }
      }
    }
  private:
    boost::array<AYM::Chip::Ptr, CHIPS> Delegates;
  };
}//TurboSound
}//Devices

namespace Devices
{
  namespace TurboSound
  {
    Chip::Ptr CreateChip(ChipParameters::Ptr params, Sound::Receiver::Ptr target)
    {
      return boost::make_shared<TurboChip>(params, target);
    }

    std::pair<AYM::Chip::Ptr, AYM::Chip::Ptr> CreateChipsPair(ChipParameters::Ptr params, Sound::Receiver::Ptr target)
    {
      const DoubleMixer::Ptr mixer = boost::make_shared<DoubleMixer>();
      return std::make_pair(
        AYM::CreateChip(params, boost::make_shared<FirstReceiver>(mixer)),
        AYM::CreateChip(params, boost::make_shared<SecondReceiver>(mixer, target))
      );
    }
  }
}
