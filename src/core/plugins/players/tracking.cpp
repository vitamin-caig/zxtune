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

  class TrackInfoImpl : public TrackInfo
  {
  public:
    explicit TrackInfoImpl(TrackModuleData::Ptr data)
      : Data(data)
      , LogicChannels(data->GetChannelsCount())
      , Frames(), LoopFrameNum()
    {
    }

    virtual uint_t PositionsCount() const
    {
      return Data->GetPositionsCount();
    }

    virtual uint_t LoopPosition() const
    {
      return Data->GetLoopPosition();
    }

    virtual uint_t PatternsCount() const
    {
      return Data->GetPatternsCount();
    }

    virtual uint_t FramesCount() const
    {
      Initialize();
      return Frames;
    }

    virtual uint_t LoopFrame() const
    {
      Initialize();
      return LoopFrameNum;
    }

    virtual uint_t LogicalChannels() const
    {
      return LogicChannels;
    }

    virtual uint_t PhysicalChannels() const
    {
      return Data->GetChannelsCount();
    }

    virtual uint_t Tempo() const
    {
      return Data->GetInitialTempo();
    }

    virtual Parameters::Accessor::Ptr Properties() const
    {
      assert(ModuleProperties);
      return ModuleProperties;
    }

    //modifiers
    virtual void SetLogicalChannels(uint_t channels)
    {
      LogicChannels = channels;
    }

    virtual void SetModuleProperties(Parameters::Accessor::Ptr props)
    {
      ModuleProperties = props;
    }

  private:
    void Initialize() const
    {
      if (Frames)
      {
        return;//initialized
      }
      //emulate playback
      const Information::Ptr dummyInfo = boost::make_shared<TrackInfoImpl>(*this);
      const TrackStateIterator::Ptr dummyIterator = TrackStateIterator::Create(dummyInfo, Data, Analyzer::Ptr());

      const uint_t loopPosNum = Data->GetLoopPosition();
      TrackStateIterator& iterator = *dummyIterator;
      while (iterator.NextFrame(0, Sound::LOOP_NONE))
      {
        //check for loop
        if (0 == iterator.Line() &&
            0 == iterator.Quirk() &&
            loopPosNum == iterator.Position())
        {
          LoopFrameNum = iterator.Frame();
        }
        //to prevent reset
        Frames = std::max(Frames, iterator.Frame());
      }
      ++Frames;
    }
  private:
    const TrackModuleData::Ptr Data;
    uint_t LogicChannels;
    Parameters::Accessor::Ptr ModuleProperties;
    mutable uint_t Frames;
    mutable uint_t LoopFrameNum;
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

    TrackInfo::Ptr TrackInfo::Create(TrackModuleData::Ptr data)
    {
      return boost::make_shared<TrackInfoImpl>(data);
    }
  }
}
