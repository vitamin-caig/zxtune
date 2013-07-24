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
//boost includes
#include <boost/array.hpp>

namespace Devices
{
  //Level in percents
  typedef Math::FixedPoint<uint_t, 100> LevelType;

  //Single channel state
  struct ChannelState
  {
    ChannelState()
      : Enabled(), Band(), Level()
    {
    }

    //Is channel enabled to output, else other fields are not set
    bool Enabled;
    //Currently played tone band (up to 96)
    uint_t Band;
    //Currently played tone level percentage
    LevelType Level;
  };

  typedef std::vector<ChannelState> MultiChannelState;
  
  template<unsigned Channels>
  class FixedChannelsCountState : public boost::array<ChannelState, Channels> {};
}

#endif //DEVICES_STATE_H_DEFINED
