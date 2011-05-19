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
#include <boost/make_shared.hpp>
#include <boost/mem_fn.hpp>

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  class DACAnalyzer : public Analyzer
  {
  public:
    explicit DACAnalyzer(Devices::DAC::Chip::Ptr device)
      : Device(device)
    {
    }

    //analyzer virtuals
    virtual uint_t ActiveChannels() const
    {
      Devices::DAC::ChannelsState state;
      Device->GetState(state);
      return static_cast<uint_t>(std::count_if(state.begin(), state.end(),
        boost::mem_fn(&Devices::DAC::ChanState::Enabled)));
    }

    virtual void BandLevels(std::vector<std::pair<uint_t, uint_t> >& bandLevels) const
    {
      Devices::DAC::ChannelsState state;
      Device->GetState(state);
      bandLevels.resize(state.size());
      std::transform(state.begin(), state.end(), bandLevels.begin(),
        boost::bind(&std::make_pair<uint_t, uint_t>, boost::bind(&Devices::DAC::ChanState::Band, _1), boost::bind(&Devices::DAC::ChanState::LevelInPercents, _1)));
    }
  private:
    const Devices::DAC::Chip::Ptr Device;
  };
}

namespace ZXTune
{
  namespace Module
  {
    Analyzer::Ptr CreateDACAnalyzer(Devices::DAC::Chip::Ptr device)
    {
      return boost::make_shared<DACAnalyzer>(device);
    }
  }
}
