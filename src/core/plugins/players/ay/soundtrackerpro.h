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

  class OrderListWithTransposition : public OrderList
  {
  public:
    typedef boost::shared_ptr<const OrderListWithTransposition> Ptr;

    template<class It>
    OrderListWithTransposition(uint_t loop, It from, It to)
      : Loop(loop)
      , Positions(from, to)
    {
    }

    virtual uint_t GetSize() const
    {
      return Positions.size();
    }

    virtual uint_t GetPatternIndex(uint_t pos) const
    {
      return Positions[pos].PatternIndex;
    }

    virtual uint_t GetLoopPosition() const
    {
      return Loop;
    }

    int_t GetTransposition(uint_t pos) const
    {
      return Positions[pos].Transposition;
    }
  private:
    const uint_t Loop;
    const std::vector<Formats::Chiptune::SoundTrackerPro::PositionEntry> Positions;
  };

  typedef SimpleOrnament Ornament;

  class ModuleData : public TrackModel
  {
  public:
    typedef boost::shared_ptr<ModuleData> RWPtr;
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

  std::auto_ptr<Formats::Chiptune::SoundTrackerPro::Builder> CreateDataBuilder(ModuleData::RWPtr data, ModuleProperties::RWPtr props);

  AYM::Chiptune::Ptr CreateChiptune(ModuleData::Ptr data, ModuleProperties::Ptr properties);
}

#endif //__CORE_PLUGINS_PLAYERS_SOUNDTRACKERPRO_DEFINED__
