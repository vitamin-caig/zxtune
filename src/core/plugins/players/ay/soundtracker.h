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

namespace SoundTracker
{
  const uint_t MAX_SAMPLES_COUNT = 16;
  const uint_t MAX_ORNAMENTS_COUNT = 16;
  const uint_t MAX_PATTERN_SIZE = 64;
  const uint_t MAX_PATTERNS_COUNT = 32;
  const uint_t SAMPLE_ORNAMENT_SIZE = 32;

  // currently used sample
  struct Sample
  {
    Sample() : Loop(), LoopLimit(), Lines()
    {
    }

    template<class T>
    Sample(uint_t loop, uint_t loopLimit, T from, T to)
      : Loop(loop)
      , LoopLimit(loopLimit)
      , Lines(from, to)
    {
    }

    struct Line
    {
      Line() : Level(), Noise(), NoiseMask(), EnvelopeMask(), Effect()
      {
      }

      uint_t Level;//0-15
      uint_t Noise;//0-31
      bool NoiseMask;
      bool EnvelopeMask;
      int_t Effect;
    };

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
  private:
    uint_t Loop;
    uint_t LoopLimit;
    std::vector<Line> Lines;
  };

  enum CmdType
  {
    EMPTY,
    ENVELOPE,     //2p
    NOENVELOPE,   //0p
  };

  typedef ZXTune::Module::TrackingSupport<Devices::AYM::CHANNELS, CmdType, Sample> Track;

  class ModuleData : public Track::ModuleData
  {
  public:
    typedef boost::shared_ptr<const ModuleData> Ptr;

    std::vector<int_t> Transpositions;
  };

  ZXTune::Module::AYM::Chiptune::Ptr CreateChiptune(ModuleData::Ptr data, ZXTune::Module::ModuleProperties::Ptr properties);
}

#endif //__CORE_PLUGINS_PLAYERS_SOUNDTRACKER_DEFINED__