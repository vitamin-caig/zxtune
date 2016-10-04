/**
* 
* @file
*
* @brief  Device state source interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <math/fixedpoint.h>
//std includes
#include <memory>
#include <vector>

namespace Devices
{
  //Level in percents
  typedef Math::FixedPoint<uint_t, 100> LevelType;

  //Single channel state
  struct ChannelState
  {
    ChannelState()
      : Band(), Level()
    {
    }

    ChannelState(uint_t band, LevelType level)
      : Band(band)
      , Level(level)
    {
    }

    //Currently played tone band (up to 96)
    uint_t Band;
    //Currently played tone level percentage
    LevelType Level;
  };

  typedef std::vector<ChannelState> MultiChannelState;

  class StateSource
  {
  public:
    typedef std::shared_ptr<const StateSource> Ptr;
    virtual ~StateSource() {}

    virtual void GetState(MultiChannelState& result) const = 0;
  };
}
