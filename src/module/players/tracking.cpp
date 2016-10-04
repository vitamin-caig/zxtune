/**
* 
* @file
*
* @brief  Track modules support implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "tracking.h"
//common includes
#include <pointers.h>
#include <make_ptr.h>
//boost includes
#include <boost/bind.hpp>

namespace Module
{
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
      return MakeSingletonPointer(instance);
    }
  };

  struct PlainTrackState
  {
    uint_t Frame;
    uint_t Position;
    uint_t Pattern;
    uint_t Line;
    uint_t Quirk;
    uint_t Tempo;

    PlainTrackState()
      : Frame(), Position(), Pattern(), Line(), Quirk(), Tempo()
    {
    }
  };

  class TrackStateCursor : public TrackModelState
  {
  public:
    typedef std::shared_ptr<TrackStateCursor> Ptr;

    explicit TrackStateCursor(TrackModel::Ptr model)
      : Model(model)
      , Order(Model->GetOrder())
      , Patterns(Model->GetPatterns())
    {
      Reset();
    }

    //TrackState
    virtual uint_t Position() const
    {
      return Plain.Position;
    }

    virtual uint_t Pattern() const
    {
      return Plain.Pattern;
    }

    virtual uint_t PatternSize() const
    {
      return CurPatternObject->GetSize();
    }

    virtual uint_t Line() const
    {
      return Plain.Line;
    }

    virtual uint_t Tempo() const
    {
      return Plain.Tempo;
    }

    virtual uint_t Quirk() const
    {
      return Plain.Quirk;
    }

    virtual uint_t Frame() const
    {
      return Plain.Frame;
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
      return Plain.Position < Order.GetSize();
    }

    const PlainTrackState& GetState() const
    {
      return Plain;
    }

    void Reset()
    {
      Plain.Frame = 0;
      Plain.Tempo = Model->GetInitialTempo();
      SetPosition(0);
    }

    void SetState(const PlainTrackState& state)
    {
      SetPosition(state.Position);
      assert(Plain.Pattern == state.Pattern);
      SetLine(state.Line);
      Plain.Quirk = state.Quirk;
      Plain.Tempo = state.Tempo;
      Plain.Frame = state.Frame;
    }

    void Seek(uint_t position)
    {
      if (Plain.Position > position ||
          (Plain.Position == position && (0 != Plain.Line || 0 != Plain.Quirk)))
      {
        Reset();
      }
      while (IsValid() && Plain.Position != position)
      {
        Plain.Frame += Plain.Tempo;
        if (!NextLine())
        {
          NextPosition();
        }
      }
    }

    bool NextFrame()
    {
      return NextQuirk() || NextLine() || NextPosition();
    }
  private:
    void SetPosition(uint_t pos)
    {
      Plain.Position = pos;
      if (IsValid())
      {
        SetPattern(Order.GetPatternIndex(Plain.Position));
      }
      else
      {
        SetStubPattern();
      }
    }

    void SetStubPattern()
    {
      Plain.Pattern = 0;
      CurPatternObject = StubPattern::Create();
      SetLine(0);
    }

    void SetPattern(uint_t pat)
    {
      Plain.Pattern = pat;
      CurPatternObject = Patterns.Get(Plain.Pattern);
      SetLine(0);
    }

    void SetLine(uint_t line)
    {
      Plain.Quirk = 0;
      Plain.Line = line;
      if (CurLineObject = CurPatternObject->GetLine(Plain.Line))
      {
        if (uint_t tempo = CurLineObject->GetTempo())
        {
          Plain.Tempo = tempo;
        }
      }
    }

    bool NextQuirk()
    {
      ++Plain.Frame;
      return ++Plain.Quirk < Plain.Tempo;
    }

    bool NextLine()
    {
      SetLine(Plain.Line + 1);
      return Plain.Line < CurPatternObject->GetSize();
    }

    bool NextPosition()
    {
      SetPosition(Plain.Position + 1);
      return IsValid();
    }
  private:
    //context
    const TrackModel::Ptr Model;
    const OrderList& Order;
    const PatternsSet& Patterns;
    //state
    PlainTrackState Plain;
    Pattern::Ptr CurPatternObject;
    Line::Ptr CurLineObject;
  };

  class TrackStateIteratorImpl : public TrackStateIterator
  {
  public:
    explicit TrackStateIteratorImpl(TrackModel::Ptr model)
      : Model(model)
      , Cursor(MakePtr<TrackStateCursor>(model))
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
      if (!Cursor->NextFrame() && looped)
      {
        MoveToLoop();
      }
    }

    virtual TrackModelState::Ptr GetStateObserver() const
    {
      return Cursor;
    }
  private:
    void MoveToLoop()
    {
      if (LoopState.get())
      {
        Cursor->SetState(*LoopState);
      }
      else
      {
        Cursor->Seek(Model->GetOrder().GetLoopPosition());
        const PlainTrackState& loop = Cursor->GetState();
        LoopState.reset(new PlainTrackState(loop));
      }
    }
  private:
    const TrackModel::Ptr Model;
    const TrackStateCursor::Ptr Cursor;
    std::unique_ptr<const PlainTrackState> LoopState;
  };

  class InformationImpl : public Information
  {
  public:
    InformationImpl(TrackModel::Ptr model, uint_t channels)
      : Model(model)
      , Channels(channels)
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

    virtual uint_t ChannelsCount() const
    {
      return Channels;
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
      TrackStateCursor cursor(Model);
      cursor.Seek(Model->GetOrder().GetLoopPosition());
      LoopFrameNum = cursor.GetState().Frame;
      cursor.Seek(Model->GetOrder().GetSize());
      Frames = cursor.GetState().Frame;
    }
  private:
    const TrackModel::Ptr Model;
    const uint_t Channels;
    mutable uint_t Frames;
    mutable uint_t LoopFrameNum;
  };

  Information::Ptr CreateTrackInfo(TrackModel::Ptr model, uint_t channels)
  {
    return MakePtr<InformationImpl>(model, channels);
  }

  TrackStateIterator::Ptr CreateTrackStateIterator(TrackModel::Ptr model)
  {
    return MakePtr<TrackStateIteratorImpl>(model);
  }
}
