/**
 *
 * @file
 *
 * @brief  Track modules support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "module/players/tracking.h"
// common includes
#include <make_ptr.h>
#include <pointers.h>
// std includes
#include <memory>

namespace Module
{
  class StubPattern : public Pattern
  {
    StubPattern() = default;

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
    uint_t Frame = 0;
    uint_t Position = 0;
    uint_t Pattern = 0;
    uint_t Line = 0;
    uint_t Quirk = 0;
    uint_t Tempo = 0;

    PlainTrackState() = default;
  };

  class TrackStateCursor : public TrackModelState
  {
  public:
    using Ptr = std::shared_ptr<TrackStateCursor>;

    TrackStateCursor(Time::Microseconds frameDuration, TrackModel::Ptr model)
      : FrameDuration(frameDuration)
      , Model(std::move(model))
      , Order(Model->GetOrder())
      , Patterns(Model->GetPatterns())
    {
      Reset();
    }

    // State
    Time::AtMillisecond At() const override
    {
      return Time::AtMillisecond() + (FrameDuration * Plain.Frame).CastTo<Time::Millisecond>();
    }

    Time::Milliseconds Total() const override
    {
      return TotalPlayed.CastTo<Time::Millisecond>();
    }

    uint_t LoopCount() const override
    {
      return Loops;
    }

    // TrackState
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

    uint_t Channels() const override
    {
      return CurLineObject ? CurLineObject->CountActiveChannels() : 0;
    }

    // TrackModelState
    const class Pattern* PatternObject() const override
    {
      return CurPatternObject;
    }

    const class Line* LineObject() const override
    {
      return CurLineObject;
    }

    // navigation
    const PlainTrackState& GetState() const
    {
      return Plain;
    }

    void Reset()
    {
      Plain.Frame = 0;
      Plain.Tempo = Model->GetInitialTempo();
      SetPosition(0);
      TotalPlayed = {};
      Loops = 0;
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
      if (Plain.Position > position || (Plain.Position == position && (0 != Plain.Line || 0 != Plain.Quirk)))
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
      if (NextQuirk() || NextLine() || NextPosition())
      {
        TotalPlayed += FrameDuration;
        return true;
      }
      else
      {
        return false;
      }
    }

    void DoneLoop()
    {
      ++Loops;
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

    bool IsValid() const
    {
      return Plain.Position < Order.GetSize();
    }

  private:
    // context
    const Time::Microseconds FrameDuration;
    const TrackModel::Ptr Model;
    const OrderList& Order;
    const PatternsSet& Patterns;
    // state
    PlainTrackState Plain;
    const class Pattern* CurPatternObject;
    const class Line* CurLineObject;
    Time::Microseconds TotalPlayed;
    uint_t Loops = 0;
  };

  class TrackStateIteratorImpl : public TrackStateIterator
  {
  public:
    TrackStateIteratorImpl(Time::Microseconds frameDuration, TrackModel::Ptr model)
      : Model(std::move(model))
      , Cursor(MakePtr<TrackStateCursor>(frameDuration, Model))
    {}

    // iterator functions
    void Reset() override
    {
      Cursor->Reset();
    }

    void NextFrame() override
    {
      if (!Cursor->NextFrame())
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
      if (LoopState)
      {
        Cursor->SetState(*LoopState);
      }
      else
      {
        Cursor->Seek(Model->GetOrder().GetLoopPosition());
        const PlainTrackState& loop = Cursor->GetState();
        LoopState = std::make_unique<PlainTrackState>(loop);
      }
      Cursor->DoneLoop();
    }

  private:
    const TrackModel::Ptr Model;
    const TrackStateCursor::Ptr Cursor;
    std::unique_ptr<const PlainTrackState> LoopState;
  };

  class TrackInformationImpl : public TrackInformation
  {
  public:
    TrackInformationImpl(Time::Microseconds frameDuration, TrackModel::Ptr model, uint_t channels)
      : FrameDuration(frameDuration)
      , Model(std::move(model))
      , Channels(channels)
    {}

    Time::Milliseconds Duration() const override
    {
      Initialize();
      return (FrameDuration * Frames).CastTo<Time::Millisecond>();
    }

    Time::Milliseconds LoopDuration() const override
    {
      Initialize();
      return (FrameDuration * (Frames - LoopFrameNum)).CastTo<Time::Millisecond>();
    }

    uint_t PositionsCount() const override
    {
      return Model->GetOrder().GetSize();
    }

    uint_t LoopPosition() const override
    {
      return Model->GetOrder().GetLoopPosition();
    }

    uint_t ChannelsCount() const override
    {
      return Channels;
    }

  private:
    void Initialize() const
    {
      if (Frames)
      {
        return;  // initialized
      }
      TrackStateCursor cursor({}, Model);
      cursor.Seek(Model->GetOrder().GetLoopPosition());
      LoopFrameNum = cursor.GetState().Frame;
      cursor.Seek(Model->GetOrder().GetSize());
      Frames = cursor.GetState().Frame;
    }

  private:
    const Time::Microseconds FrameDuration;
    const TrackModel::Ptr Model;
    const uint_t Channels;
    mutable uint_t Frames = 0;
    mutable uint_t LoopFrameNum = 0;
  };

  TrackInformation::Ptr CreateTrackInfoFixedChannels(Time::Microseconds frameDuration, TrackModel::Ptr model,
                                                     uint_t channels)
  {
    return MakePtr<TrackInformationImpl>(frameDuration, std::move(model), channels);
  }

  TrackStateIterator::Ptr CreateTrackStateIterator(Time::Microseconds frameDuration, TrackModel::Ptr model)
  {
    return MakePtr<TrackStateIteratorImpl>(frameDuration, std::move(model));
  }
}  // namespace Module
