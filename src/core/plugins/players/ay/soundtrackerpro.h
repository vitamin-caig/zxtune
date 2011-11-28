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

  struct Ornament : public Formats::Chiptune::SoundTrackerPro::Ornament
  {
    Ornament() 
      : Formats::Chiptune::SoundTrackerPro::Ornament()
    {
    }

    Ornament(const Formats::Chiptune::SoundTrackerPro::Ornament& rh)
      : Formats::Chiptune::SoundTrackerPro::Ornament(rh)
    {
    }

    uint_t GetLoop() const
    {
      return Loop;
    }

    uint_t GetSize() const
    {
      return static_cast<uint_t>(Lines.size());
    }

    int_t GetLine(uint_t pos) const
    {
      return Lines.size() > pos ? Lines[pos] : 0;
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

  std::auto_ptr<Formats::Chiptune::SoundTrackerPro::Builder> CreateDataBuilder(ModuleData::RWPtr data, ZXTune::Module::ModuleProperties::RWPtr props);

  ZXTune::Module::AYM::Chiptune::Ptr CreateChiptune(ModuleData::Ptr data, ZXTune::Module::ModuleProperties::Ptr properties);
}

#endif //__CORE_PLUGINS_PLAYERS_SOUNDTRACKERPRO_DEFINED__
