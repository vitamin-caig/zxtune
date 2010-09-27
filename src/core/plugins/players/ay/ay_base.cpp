/*
Abstract:
  AYM-based players common functionality implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "ay_base.h"
//std includes
#include <limits>
//boost includes
#include <boost/bind.hpp>
#include <boost/mem_fn.hpp>

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  class AYMDeviceImpl : public AYMDevice
  {
  public:
    explicit AYMDeviceImpl(AYM::Chip::Ptr device)
      : Device(device)
      , CurState(0)
    {
    }

    //analyzer virtuals
    virtual uint_t ActiveChannels() const
    {
      FillState();
      return std::count_if(StateCache.begin(), StateCache.end(),
        boost::mem_fn(&AYM::ChanState::Enabled));
    }

    virtual void BandLevels(std::vector<std::pair<uint_t, uint_t> >& bandLevels) const
    {
      FillState();
      bandLevels.resize(StateCache.size());
      std::transform(StateCache.begin(), StateCache.end(), bandLevels.begin(),
        boost::bind(&std::make_pair<uint_t, uint_t>, boost::bind(&AYM::ChanState::Band, _1), boost::bind(&AYM::ChanState::LevelInPercents, _1)));
    }

    virtual void RenderData(const Sound::RenderParameters& params,
                            const AYM::DataChunk& src,
                            Sound::MultichannelReceiver& dst)
    {
      Device->RenderData(params, src, dst);
      CurState = 0;
    }

    virtual void Reset()
    {
      Device->Reset();
      CurState = 0;
    }
  private:
    void FillState() const
    {
      if (!CurState)
      {
        Device->GetState(StateCache);
        CurState = &StateCache;
      }
    }
  private:
    const AYM::Chip::Ptr Device;
    mutable AYM::ChannelsState* CurState;
    mutable AYM::ChannelsState StateCache;
  };
}

namespace ZXTune
{
  namespace Module
  {
    AYMDevice::Ptr AYMDevice::Create(AYM::Chip::Ptr device)
    {
      return AYMDevice::Ptr(new AYMDeviceImpl(device));
    }
  }
}
