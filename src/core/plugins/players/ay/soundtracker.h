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
#include "core/plugins/players/module_properties.h"
#include "core/plugins/players/simple_orderlist.h"
#include "core/plugins/players/simple_ornament.h"
//library includes
#include <formats/chiptune/aym/soundtracker.h>

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

  typedef SimpleOrderListWithTransposition<Formats::Chiptune::SoundTracker::PositionEntry> OrderListWithTransposition;
  typedef SimpleOrnament Ornament;

  class ModuleData : public TrackModel
  {
  public:
    typedef boost::shared_ptr<const ModuleData> Ptr;

    ModuleData()
      : InitialTempo()
    {
    }

    virtual uint_t GetInitialTempo() const
    {
      return InitialTempo;
    }

    virtual const OrderList& GetOrder() const
    {
      return *Order;
    }

    virtual const PatternsSet& GetPatterns() const
    {
      return *Patterns;
    }

    uint_t InitialTempo;
    OrderListWithTransposition::Ptr Order;
    PatternsSet::Ptr Patterns;
    SparsedObjectsStorage<Sample> Samples;
    SparsedObjectsStorage<Ornament> Ornaments;
  };

  class DataBuilder : public Formats::Chiptune::SoundTracker::Builder
  {
  public:
    virtual ModuleData::Ptr GetResult() const = 0;
  };

  std::auto_ptr<DataBuilder> CreateDataBuilder(PropertiesBuilder& propBuilder);

  AYM::Chiptune::Ptr CreateChiptune(ModuleData::Ptr data, Parameters::Accessor::Ptr properties);
}

#endif //__CORE_PLUGINS_PLAYERS_SOUNDTRACKER_DEFINED__