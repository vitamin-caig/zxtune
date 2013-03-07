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

  class StubPattern : public Pattern
  {
    StubPattern()
    {
    }
  public:
    virtual Line::Ptr GetLine(uint_t /*row*/) const
    {
      return Line::Ptr();
    }

    virtual uint_t GetSize() const
    {
      return 0;
    }

    static Ptr Create()
    {
      static StubPattern instance;
      return Ptr(&instance, NullDeleter<Pattern>());
    }
  };

  class TrackStateCursor : public TrackModelState
  {
  public:
    typedef boost::shared_ptr<TrackStateCursor> Ptr;

    TrackStateCursor(Information::Ptr info, TrackModel::Ptr model)
      : Info(info)
      , Model(model)
      , Order(Model->GetOrder())
      , Patterns(Model->GetPatterns())
      , CurPosition()
      , CurPattern()
      , CurLine()
      , CurTempo()
      , CurQuirk()
      , CurFrame()
    {
      Reset();
    }

    //TrackState
    virtual uint_t Position() const
    {
      return CurPosition;
    }

    virtual uint_t Pattern() const
    {
      return CurPattern;
    }

    virtual uint_t PatternSize() const
    {
      return CurPatternObject->GetSize();
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
      return CurLineObject ? CurLineObject->CountActiveChannels() : 0;
    }

    //TrackModelState
    virtual Pattern::Ptr PatternObject() const
    {
      return CurPatternObject;
    }

    virtual Line::Ptr LineObject() const
    {
      return CurLineObject;
    }

    //navigation
    bool IsValid() const
    {
      return CurPosition < Order.GetSize();
    }

    void Reset()
    {
      Reset(0, 0);
    }

    void ResetToLoop()
    {
      Reset(Order.GetLoopPosition(), Info->LoopFrame());
    }

    bool NextQuirk()
    {
      ++CurFrame;
      return ++CurQuirk < CurTempo;
    }

    bool NextLine()
    {
      SetLine(CurLine + 1);
      return CurLine < CurPatternObject->GetSize();
    }

    bool NextPosition()
    {
      SetPosition(CurPosition + 1);
      return IsValid();
    }
  private:
    void Reset(uint_t pos, uint_t frame)
    {
      CurFrame = frame;
      CurTempo = Info->Tempo();
      //in case if frame is 0, positions should be 0 too
      SetPosition(frame != 0 ? pos : 0);
    }

    void SetPosition(uint_t pos)
    {
      CurPosition = pos;
      if (IsValid())
      {
        SetPattern(Order.GetPatternIndex(CurPosition));
      }
      else
      {
        SetStubPattern();
      }
    }

    void SetStubPattern()
    {
      CurPattern = 0;
      CurPatternObject = StubPattern::Create();
      SetLine(0);
    }

    void SetPattern(uint_t pat)
    {
      CurPatternObject = Patterns.Get(CurPattern = pat);
      SetLine(0);
    }

    void SetLine(uint_t line)
    {
      CurQuirk = 0;
      if (CurLineObject = CurPatternObject->GetLine(CurLine = line))
      {
        if (uint_t tempo = CurLineObject->GetTempo())
        {
          CurTempo = tempo;
        }
      }
    }
  private:
    //context
    const Information::Ptr Info;
    const TrackModel::Ptr Model;
    const OrderList& Order;
    const PatternsSet& Patterns;
    //state
    uint_t CurPosition;
    uint_t CurPattern;
    Pattern::Ptr CurPatternObject;
    uint_t CurLine;
    Line::Ptr CurLineObject;
    uint_t CurTempo;
    uint_t CurQuirk;
    uint_t CurFrame;
  };

  class TrackStateIteratorImpl : public TrackStateIterator
  {
  public:
    TrackStateIteratorImpl(Information::Ptr info, TrackModel::Ptr model)
      : Cursor(boost::make_shared<TrackStateCursor>(info, model))
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

    virtual TrackModelState::Ptr GetStateObserver() const
    {
      return Cursor;
    }
  private:
    const TrackStateCursor::Ptr Cursor;
  };

  class InformationImpl : public Information
  {
  public:
    InformationImpl(TrackModel::Ptr model, uint_t logicalChannels)
      : Model(model)
      , LogicChannels(logicalChannels)
      , Frames(), LoopFrameNum()
    {
    }

    virtual uint_t PositionsCount() const
    {
      return Model->GetOrder().GetSize();
    }

    virtual uint_t LoopPosition() const
    {
      return Model->GetOrder().GetLoopPosition();
    }

    virtual uint_t PatternsCount() const
    {
      return Model->GetPatterns().GetSize();
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
      return Model->GetChannelsCount();
    }

    virtual uint_t Tempo() const
    {
      return Model->GetInitialTempo();
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
      const TrackStateIterator::Ptr dummyIterator = CreateTrackStateIterator(dummyInfo, Model);
      const TrackState::Ptr dummyState = dummyIterator->GetStateObserver();

      const uint_t loopPosNum = Model->GetOrder().GetLoopPosition();
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
    const TrackModel::Ptr Model;
    const uint_t LogicChannels;
    mutable uint_t Frames;
    mutable uint_t LoopFrameNum;
  };
}

namespace ZXTune
{
  namespace Module
  {
    Information::Ptr CreateTrackInfo(TrackModel::Ptr model, uint_t logicalChannels)
    {
      return boost::make_shared<InformationImpl>(model, logicalChannels);
    }

    TrackStateIterator::Ptr CreateTrackStateIterator(Information::Ptr info, TrackModel::Ptr model)
    {
      return boost::make_shared<TrackStateIteratorImpl>(info, model);
    }
  }
}
