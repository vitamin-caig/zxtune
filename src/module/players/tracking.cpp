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

namespace Module
{
  class StubPattern : public Pattern
  {
    StubPattern()
    {
    }
  public:
    const Line* GetLine(uint_t /*row*/) const override
    {
      return nullptr;
    }

    uint_t GetSize() const override
    {
      return 0;
    }

    static const Pattern* Create()
    {
      static const StubPattern instance;
      return &instance;
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
      : Model(std::move(model))
      , Order(Model->GetOrder())
      , Patterns(Model->GetPatterns())
    {
      Reset();
    }

    //TrackState
    uint_t Position() const override
    {
      return Plain.Position;
    }

    uint_t Pattern() const override
    {
      return Plain.Pattern;
    }

    uint_t Line() const override
    {
      return Plain.Line;
    }

    uint_t Tempo() const override
    {
      return Plain.Tempo;
    }

    uint_t Quirk() const override
    {
      return Plain.Quirk;
    }

    uint_t Frame() const override
    {
      return Plain.Frame;
    }

    uint_t Channels() const override
    {
      return CurLineObject ? CurLineObject->CountActiveChannels() : 0;
    }

    //TrackModelState
    const class Pattern* PatternObject() const override
    {
      return CurPatternObject;
    }

    const class Line* LineObject() const override
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
      CurLineObject = CurPatternObject->GetLine(Plain.Line);
      if (CurLineObject)
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
    const class Pattern* CurPatternObject;
    const class Line* CurLineObject;
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
    void Reset() override
    {
      Cursor->Reset();
    }

    bool IsValid() const override
    {
      return Cursor->IsValid();
    }

    void NextFrame(bool looped) override
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

    TrackModelState::Ptr GetStateObserver() const override
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
      : Model(std::move(model))
      , Channels(channels)
      , Frames(), LoopFrameNum()
    {
    }

    uint_t PositionsCount() const override
    {
      return Model->GetOrder().GetSize();
    }

    uint_t LoopPosition() const override
    {
      return Model->GetOrder().GetLoopPosition();
    }

    uint_t FramesCount() const override
    {
      Initialize();
      return Frames;
    }

    uint_t LoopFrame() const override
    {
      Initialize();
      return LoopFrameNum;
    }

    uint_t ChannelsCount() const override
    {
      return Channels;
    }

    uint_t Tempo() const override
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
    return MakePtr<InformationImpl>(std::move(model), channels);
  }

  TrackStateIterator::Ptr CreateTrackStateIterator(TrackModel::Ptr model)
  {
    return MakePtr<TrackStateIteratorImpl>(std::move(model));
  }
}
