/*
Abstract:
  SoundTracker support interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __CORE_PLUGINS_PLAYERS_SOUNDTRACKER_DEFINED__
#define __CORE_PLUGINS_PLAYERS_SOUNDTRACKER_DEFINED__

//local includes
#include "ay_base.h"
//library includes
#include <formats/chiptune/soundtracker.h>

namespace SoundTracker
{
  enum CmdType
  {
    EMPTY,
    ENVELOPE,     //2p
    NOENVELOPE,   //0p
  };

  struct Sample : public Formats::Chiptune::SoundTracker::Sample
  {
    Sample()
      : Formats::Chiptune::SoundTracker::Sample()
    {
    }

    Sample(const Formats::Chiptune::SoundTracker::Sample& rh)
      : Formats::Chiptune::SoundTracker::Sample(rh)
    {
    }

    uint_t GetLoop() const
    {
      return Loop;
    }

    uint_t GetLoopLimit() const
    {
      return LoopLimit;
    }

    uint_t GetSize() const
    {
      return static_cast<uint_t>(Lines.size());
    }

    const Line& GetLine(uint_t idx) const
    {
      static const Line STUB;
      return Lines.size() > idx ? Lines[idx] : STUB;
    }
  };

  using namespace ZXTune;
  using namespace ZXTune::Module;
  //SimpleOrnament::Loop is not use
  typedef TrackingSupport<Devices::AYM::CHANNELS, Sample, SimpleOrnament, OrderListWithTransposition> Track;

  std::auto_ptr<Formats::Chiptune::SoundTracker::Builder> CreateDataBuilder(Track::ModuleData::RWPtr data, ModuleProperties::RWPtr props);

  AYM::Chiptune::Ptr CreateChiptune(Track::ModuleData::Ptr data, ModuleProperties::Ptr properties);
}

#endif //__CORE_PLUGINS_PLAYERS_SOUNDTRACKER_DEFINED__