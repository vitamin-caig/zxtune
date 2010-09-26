/*
Abstract:
  DAC-based players common functionality implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "dac_base.h"
//std includes
#include <limits>
//boost includes
#include <boost/bind.hpp>
#include <boost/mem_fn.hpp>

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  class DACDeviceImpl : public DACDevice
  {
  public:
    explicit DACDeviceImpl(DAC::Chip::Ptr device)
      : Device(device)
      , CurState(0)
    {
    }

    //analyzer virtuals
    virtual uint_t ActiveChannels() const
    {
      FillState();
      return std::count_if(StateCache.begin(), StateCache.end(),
        boost::mem_fn(&DAC::ChanState::Enabled));
    }

    virtual void BandLevels(std::vector<std::pair<uint_t, uint_t> >& bandLevels) const
    {
      FillState();
      bandLevels.resize(StateCache.size());
      std::transform(StateCache.begin(), StateCache.end(), bandLevels.begin(),
        boost::bind(&std::make_pair<uint_t, uint_t>, boost::bind(&DAC::ChanState::Band, _1), boost::bind(&DAC::ChanState::LevelInPercents, _1)));
    }

    virtual void RenderData(const Sound::RenderParameters& params,
                            const DAC::DataChunk& src,
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
    const DAC::Chip::Ptr Device;
    mutable DAC::ChannelsState* CurState;
    mutable DAC::ChannelsState StateCache;
  };
}

namespace ZXTune
{
  namespace Module
  {
    DACDevice::Ptr DACDevice::Create(DAC::Chip::Ptr device)
    {
      return DACDevice::Ptr(new DACDeviceImpl(device));
    }
  }
}
