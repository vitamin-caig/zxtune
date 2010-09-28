/*
Abstract:
  Tracked modules support implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "tracking.h"
//boost includes
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  class TrackStateIteratorImpl : public TrackStateIterator
  {
  public:
    TrackStateIteratorImpl(Information::Ptr info, TrackModuleData::Ptr data, Analyzer::Ptr analyze)
      : Info(info), Data(data), Analyze(analyze)
    {
      Reset();
    }

    //status functions
    virtual uint_t Position() const
    {
      return CurPosition;
    }

    virtual uint_t Pattern() const
    {
      return Data->GetCurrentPattern(*this);
    }

    virtual uint_t PatternSize() const
    {
      return Data->GetCurrentPatternSize(*this);
    }

    virtual uint_t Line() const
    {
      return CurLine;
    }

    virtual uint_t Tempo() const
    {
      return CurTempo;
    }

    virtual uint_t Quirk() const
    {
      return CurQuirk;
    }

    virtual uint_t Frame() const
    {
      return CurFrame;
    }

    virtual uint_t Channels() const
    {
      return Analyze->ActiveChannels();
    }

    virtual uint_t AbsoluteFrame() const
    {
      return AbsFrame;
    }

    virtual uint64_t AbsoluteTick() const
    {
      return AbsTick;
    }

    //iterator functions
    virtual void Reset()
    {
      AbsFrame = 0;
      AbsTick = 0;
      ResetPosition();
    }

    virtual void ResetPosition()
    {
      CurFrame = 0;
      CurPosition = 0;
      CurLine = 0;
      CurQuirk = 0;
      if (!UpdateTempo())
      {
        CurTempo = Info->Tempo();
      }
    }

    virtual bool NextFrame(uint64_t ticksToSkip, Sound::LoopMode mode)
    {
      ++CurFrame;
      ++AbsFrame;
      AbsTick += ticksToSkip;
      if (++CurQuirk >= Tempo() &&
          !NextLine(mode))
      {
        return false;
      }
      return true;
    }
  private:
    bool NextLine(Sound::LoopMode mode)
    {
      CurQuirk = 0;
      if (++CurLine >= PatternSize() &&
          !NextPosition(mode))
      {
        return false;
      }
      UpdateTempo();
      return true;
    }

    bool NextPosition(Sound::LoopMode mode)
    {
      CurLine = 0;
      if (++CurPosition >= Info->PositionsCount() &&
          !ProcessLoop(mode))
      {
        return false;
      }
      return true;
    }
  private:
    bool UpdateTempo()
    {
      if (uint_t tempo = Data->GetNewTempo(*this))
      {
        CurTempo = tempo;
        return true;
      }
      return false;
    }

    bool ProcessLoop(Sound::LoopMode mode)
    {
      switch (mode)
      {
      case Sound::LOOP_NORMAL:
        return ProcessNormalLoop();
      case Sound::LOOP_NONE:
        return ProcessNoLoop();
      case Sound::LOOP_BEGIN:
        return ProcessBeginLoop();
      default:
        assert(!"Invalid mode");
        return false;
      }
    }

    bool ProcessNormalLoop()
    {
      CurFrame = Info->LoopFrame();
      CurPosition = Info->LoopPosition();
      //++CurLoopsCount;
      return true;
    }

    bool ProcessNoLoop()
    {
      ResetPosition();
      return false;
    }

    bool ProcessBeginLoop()
    {
      ResetPosition();
      return true;
    }
  private:
    //context
    const Information::Ptr Info;
    const TrackModuleData::Ptr Data;
    const Analyzer::Ptr Analyze;
    //state
    uint_t CurPosition;
    uint_t CurLine;
    uint_t CurTempo;
    uint_t CurQuirk;
    uint_t CurFrame;
    uint_t AbsFrame;
    uint64_t AbsTick;
  };
}

namespace ZXTune
{
  namespace Module
  {
    TrackStateIterator::Ptr TrackStateIterator::Create(
      Information::Ptr info, TrackModuleData::Ptr data, Analyzer::Ptr analyze)
    {
      return boost::make_shared<TrackStateIteratorImpl>(info, data, analyze);
    }
  }
}
