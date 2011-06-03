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

  class TrackStateIterator : public StateIterator
  {
  public:
    TrackStateIterator(Information::Ptr info, TrackModuleData::Ptr data)
      : Info(info), Data(data)
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
      return Data->GetPatternIndex(Position());
    }

    virtual uint_t PatternSize() const
    {
      return Data->GetPatternSize(Position());
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
      return Data->GetActiveChannels(Position(), Line());
    }

    //iterator functions
    virtual void Reset()
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

    virtual bool NextFrame(bool looped)
    {
      ++CurFrame;
      if (++CurQuirk >= Tempo() &&
          !NextLine(looped))
      {
        return false;
      }
      return true;
    }
  private:
    bool NextLine(bool looped)
    {
      CurQuirk = 0;
      if (++CurLine >= PatternSize() &&
          !NextPosition(looped))
      {
        return false;
      }
      UpdateTempo();
      return true;
    }

    bool NextPosition(bool looped)
    {
      CurLine = 0;
      if (++CurPosition >= Info->PositionsCount() &&
          !ProcessLoop(looped))
      {
        return false;
      }
      return true;
    }
  private:
    bool UpdateTempo()
    {
      if (uint_t tempo = Data->GetNewTempo(Position(), Line()))
      {
        CurTempo = tempo;
        return true;
      }
      return false;
    }

    bool ProcessLoop(bool looped)
    {
      return looped
        ? ProcessNormalLoop()
        : ProcessNoLoop();
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
      Reset();
      return false;
    }
  private:
    //context
    const Information::Ptr Info;
    const TrackModuleData::Ptr Data;
    //state
    uint_t CurPosition;
    uint_t CurLine;
    uint_t CurTempo;
    uint_t CurQuirk;
    uint_t CurFrame;
  };

  class InformationImpl : public Information
  {
  public:
    InformationImpl(TrackModuleData::Ptr data, uint_t logicalChannels)
      : Data(data)
      , LogicChannels(logicalChannels)
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
  private:
    void Initialize() const
    {
      if (Frames)
      {
        return;//initialized
      }
      //emulate playback
      const Information::Ptr dummyInfo = boost::make_shared<InformationImpl>(*this);
      const TrackStateIterator::Ptr dummyIterator = CreateTrackStateIterator(dummyInfo, Data);

      const uint_t loopPosNum = Data->GetLoopPosition();
      StateIterator& iterator = *dummyIterator;
      while (iterator.NextFrame(false))
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
    const uint_t LogicChannels;
    mutable uint_t Frames;
    mutable uint_t LoopFrameNum;
  };
}

namespace ZXTune
{
  namespace Module
  {
    Information::Ptr CreateTrackInfo(TrackModuleData::Ptr data, uint_t logicalChannels)
    {
      return boost::make_shared<InformationImpl>(data, logicalChannels);
    }

    StateIterator::Ptr CreateTrackStateIterator(Information::Ptr info, TrackModuleData::Ptr data)
    {
      return boost::make_shared<TrackStateIterator>(info, data);
    }
  }
}
