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

  class TrackStateCursor : public TrackState
  {
  public:
    typedef boost::shared_ptr<TrackStateCursor> Ptr;

    TrackStateCursor(Information::Ptr info, TrackModuleData::Ptr data)
      : Info(info)
      , Data(data)
      , CurPosition()
      , CurLine()
      , CurTempo()
      , CurQuirk()
      , CurFrame()
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
      return IsValid() ? Data->GetPatternIndex(Position()) : 0;
    }

    virtual uint_t PatternSize() const
    {
      return IsValid() ? Data->GetPatternSize(Position()) : 0;
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
      return IsValid() ? Data->GetActiveChannels(Position(), Line()) : 0;
    }

    //navigation
    bool IsValid() const
    {
      return CurPosition < Info->PositionsCount();
    }

    void Reset()
    {
      Reset(0, 0);
    }

    void ResetToLoop()
    {
      Reset(Info->LoopPosition(), Info->LoopFrame());
    }

    bool NextQuirk()
    {
      ++CurFrame;
      return ++CurQuirk < Tempo();
    }

    bool NextLine()
    {
      CurQuirk = 0;
      if (++CurLine >= PatternSize())
      {
        return false;
      }
      UpdateTempo();
      return true;
    }

    bool NextPosition()
    {
      CurQuirk = 0;
      CurLine = 0;
      ++CurPosition;
      if (!IsValid())
      {
        return false;
      }
      UpdateTempo();
      return true;
    }
  private:
    void Reset(uint_t pos, uint_t frame)
    {
      CurFrame = frame;
      //in case if frame is 0, positions should be 0 too
      CurPosition = frame != 0 ? pos : 0;
      CurLine = 0;
      CurQuirk = 0;
      if (!UpdateTempo())
      {
        CurTempo = Info->Tempo();
      }
    }

    bool UpdateTempo()
    {
      if (uint_t tempo = Data->GetNewTempo(Position(), Line()))
      {
        CurTempo = tempo;
        return true;
      }
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

  class TrackStateIterator : public StateIterator
  {
  public:
    TrackStateIterator(Information::Ptr info, TrackModuleData::Ptr data)
      : Cursor(boost::make_shared<TrackStateCursor>(info, data))
    {
    }

    //iterator functions
    virtual void Reset()
    {
      Cursor->Reset();
    }

    virtual bool IsValid() const
    {
      return Cursor->IsValid();
    }

    virtual void NextFrame(bool looped)
    {
      if (!Cursor->IsValid())
      {
        return;
      }
      if (Cursor->NextQuirk())
      {
        return;
      }
      if (Cursor->NextLine())
      {
        return;
      }
      if (Cursor->NextPosition())
      {
        return;
      }
      if (looped)
      {
        Cursor->ResetToLoop();
      }
    }

    virtual TrackState::Ptr GetStateObserver() const
    {
      return Cursor;
    }
  private:
    const TrackStateCursor::Ptr Cursor;
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
      const TrackState::Ptr dummyState = dummyIterator->GetStateObserver();

      const uint_t loopPosNum = Data->GetLoopPosition();
      while (dummyIterator->IsValid())
      {
        //check for loop
        if (0 == dummyState->Line() &&
            0 == dummyState->Quirk() &&
            loopPosNum == dummyState->Position())
        {
          LoopFrameNum = dummyState->Frame();
        }
        dummyIterator->NextFrame(false);
      }
      Frames = dummyState->Frame();
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
