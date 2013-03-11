/*
Abstract:
  SoundTrackerPro support interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __CORE_PLUGINS_PLAYERS_SOUNDTRACKERPRO_DEFINED__
#define __CORE_PLUGINS_PLAYERS_SOUNDTRACKERPRO_DEFINED__

//local includes
#include "ay_base.h"
//library includes
#include <formats/chiptune/soundtrackerpro.h>

namespace SoundTrackerPro
{
  enum CmdType
  {
    EMPTY,
    ENVELOPE,     //2p
    NOENVELOPE,   //0p
    GLISS,        //1p
  };

  struct Sample : public Formats::Chiptune::SoundTrackerPro::Sample
  {
    Sample()
      : Formats::Chiptune::SoundTrackerPro::Sample()
    {
    }

    Sample(const Formats::Chiptune::SoundTrackerPro::Sample& rh)
      : Formats::Chiptune::SoundTrackerPro::Sample(rh)
    {
    }

    int_t GetLoop() const
    {
      return Loop;
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

  typedef TrackingSupport<Devices::AYM::CHANNELS, Sample, SimpleOrnament, OrderListWithTransposition> Track;

  std::auto_ptr<Formats::Chiptune::SoundTrackerPro::Builder> CreateDataBuilder(Track::ModuleData::RWPtr data, ZXTune::Module::ModuleProperties::RWPtr props);

  ZXTune::Module::AYM::Chiptune::Ptr CreateChiptune(Track::ModuleData::Ptr data, ZXTune::Module::ModuleProperties::Ptr properties);
}

#endif //__CORE_PLUGINS_PLAYERS_SOUNDTRACKERPRO_DEFINED__
