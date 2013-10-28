/*
Abstract:
  Track model interfaces

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef CORE_PLUGINS_PLAYERS_TRACK_MODEL_H_DEFINED
#define CORE_PLUGINS_PLAYERS_TRACK_MODEL_H_DEFINED

//common includes
#include <iterator.h>
#include <types.h>
//std includes
#include <vector>

namespace Module
{
  struct Command
  {
    Command() : Type(), Param1(), Param2(), Param3()
    {
    }

    Command(uint_t type, int_t p1, int_t p2, int_t p3)
      : Type(type), Param1(p1), Param2(p2), Param3(p3)
    {
    }

    bool operator == (uint_t type) const
    {
      return Type == type;
    }

    uint_t Type;
    int_t Param1;
    int_t Param2;
    int_t Param3;
  };

  typedef std::vector<Command> CommandsArray;
  typedef RangeIterator<CommandsArray::const_iterator> CommandsIterator;

  class Cell
  {
  public:
    typedef const Cell* Ptr;

    Cell() : Mask(), Enabled(), Note(), SampleNum(), OrnamentNum(), Volume(), Commands()
    {
    }

    bool HasData() const
    {
      return 0 != Mask || !Commands.empty();
    }

    const bool* GetEnabled() const
    {
      return 0 != (Mask & ENABLED) ? &Enabled : 0;
    }

    const uint_t* GetNote() const
    {
      return 0 != (Mask & NOTE) ? &Note : 0;
    }

    const uint_t* GetSample() const
    {
      return 0 != (Mask & SAMPLENUM) ? &SampleNum : 0;
    }

    const uint_t* GetOrnament() const
    {
      return 0 != (Mask & ORNAMENTNUM) ? &OrnamentNum : 0;
    }

    const uint_t* GetVolume() const
    {
      return 0 != (Mask & VOLUME) ? &Volume : 0;
    }

    CommandsIterator GetCommands() const
    {
      return CommandsIterator(Commands.begin(), Commands.end());
    }
  protected:
    enum Flags
    {
      ENABLED = 1,
      NOTE = 2,
      SAMPLENUM = 4,
      ORNAMENTNUM = 8,
      VOLUME = 16
    };

    uint_t Mask;
    bool Enabled;
    uint_t Note;
    uint_t SampleNum;
    uint_t OrnamentNum;
    uint_t Volume;
    CommandsArray Commands;
  };

  class Line
  {
  public:
    typedef boost::shared_ptr<const Line> Ptr;
    virtual ~Line() {}

    virtual Cell::Ptr GetChannel(uint_t idx) const = 0;
    virtual uint_t CountActiveChannels() const = 0;
    virtual uint_t GetTempo() const = 0;
  };

  class Pattern
  {
  public:
    typedef boost::shared_ptr<const Pattern> Ptr;
    virtual ~Pattern() {}

    virtual Line::Ptr GetLine(uint_t row) const = 0;
    virtual uint_t GetSize() const = 0;
  };

  class PatternsSet
  {
  public:
    typedef boost::shared_ptr<const PatternsSet> Ptr;
    virtual ~PatternsSet() {}

    virtual Pattern::Ptr Get(uint_t idx) const = 0;
    virtual uint_t GetSize() const = 0;
  };

  class OrderList
  {
  public:
    typedef boost::shared_ptr<const OrderList> Ptr;

    virtual uint_t GetSize() const = 0;
    virtual uint_t GetPatternIndex(uint_t pos) const = 0;
    virtual uint_t GetLoopPosition() const = 0;
  };

  class TrackModel
  {
  public:
    typedef boost::shared_ptr<const TrackModel> Ptr;
    virtual ~TrackModel() {}

    virtual uint_t GetInitialTempo() const = 0;
    virtual const OrderList& GetOrder() const = 0;
    virtual const PatternsSet& GetPatterns() const = 0;
  };

  class TrackModelState : public TrackState
  {
  public:
    typedef boost::shared_ptr<const TrackModelState> Ptr;
    virtual ~TrackModelState() {}

    virtual Pattern::Ptr PatternObject() const = 0;
    virtual Line::Ptr LineObject() const = 0;
  };
}

#endif //CORE_PLUGINS_PLAYERS_TRACK_MODEL_H_DEFINED
