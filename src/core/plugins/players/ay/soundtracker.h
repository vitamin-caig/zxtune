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

  struct Ornament : public Formats::Chiptune::SoundTracker::Ornament
  {
    Ornament() 
      : Formats::Chiptune::SoundTracker::Ornament()
    {
    }

    Ornament(const Formats::Chiptune::SoundTracker::Ornament& rh)
      : Formats::Chiptune::SoundTracker::Ornament(rh)
    {
    }

    uint_t GetSize() const
    {
      return static_cast<uint_t>(size());
    }

    int_t GetLine(uint_t pos) const
    {
      return size() > pos ? at(pos) : 0;
    }
  };

  typedef ZXTune::Module::TrackingSupport<Devices::AYM::CHANNELS, CmdType, Sample, Ornament> Track;

  class ModuleData : public Track::ModuleData
  {
  public:
    typedef boost::shared_ptr<ModuleData> RWPtr;
    typedef boost::shared_ptr<const ModuleData> Ptr;

    std::vector<int_t> Transpositions;
  };

  std::auto_ptr<Formats::Chiptune::SoundTracker::Builder> CreateDataBuilder(ModuleData::RWPtr data, ZXTune::Module::ModuleProperties::RWPtr props);

  ZXTune::Module::AYM::Chiptune::Ptr CreateChiptune(ModuleData::Ptr data, ZXTune::Module::ModuleProperties::Ptr properties);
}

#endif //__CORE_PLUGINS_PLAYERS_SOUNDTRACKER_DEFINED__