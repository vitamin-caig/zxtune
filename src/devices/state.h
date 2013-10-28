/*
Abstract:
  Device's state-related declarations

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef DEVICES_STATE_H_DEFINED
#define DEVICES_STATE_H_DEFINED

//library includes
#include <math/fixedpoint.h>
//std includes
#include <vector>
//boost includes
#include <boost/shared_ptr.hpp>

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
    typedef boost::shared_ptr<const StateSource> Ptr;
    virtual ~StateSource() {}

    virtual void GetState(MultiChannelState& result) const = 0;
  };
}

#endif //DEVICES_STATE_H_DEFINED
