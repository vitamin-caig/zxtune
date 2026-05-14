/**
 *
 * @file
 *
 * @brief  Track modules support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/tracking.h"

#include "make_ptr.h"
#include "pointers.h"

#include <memory>

namespace Module
{
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

  class TrackStateCursor
  {
  public:
    TrackStateCursor(Time::Microseconds frameDuration, const TrackModel& model)
      : FrameDuration(frameDuration)
      , Model(model)
      , Order(Model.GetOrder())
      , Patterns(Model.GetPatterns())
    {
      Reset();
    }

    Time::AtMillisecond At() const
    {
      return Time::AtMillisecond() + (FrameDuration * Plain.Frame).CastTo<Time::Millisecond>();
    }

    State Get() const
    {
      return {.At = At(),
              .Total = TotalPlayed.CastTo<Time::Millisecond>(),
              .LoopCount = Loops,
              .Track = {{.Position = Plain.Position,
                         .Pattern = Plain.Pattern,
                         .Line = Plain.Line,
                         .Tempo = Plain.Tempo,
                         .Quirk = Plain.Quirk,
                         .Channels = Model.CountActiveChannels({.Pattern = Plain.Pattern, .Line = Plain.Line})}}};
    }

    // navigation
    const PlainTrackState& GetState() const
    {
      return Plain;
    }

    void Reset()
    {
      Plain.Frame = 0;
      Plain.Tempo = Model.GetInitialTempo();
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
      SetLine(0);
    }

    void SetPattern(uint_t pat)
    {
      Plain.Pattern = pat;
      SetLine(0);
    }

    void SetLine(uint_t line)
    {
      Plain.Quirk = 0;
      Plain.Line = line;
      if (const auto tempo = Model.GetLineTempo({.Pattern = Plain.Pattern, .Line = Plain.Line}))
      {
        Plain.Tempo = tempo;
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
      return Model.IsValidLine({.Pattern = Plain.Pattern, .Line = Plain.Line});
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
    const TrackModel& Model;
    const OrderList& Order;
    const PatternsSet& Patterns;
    // state
    PlainTrackState Plain;
    Time::Microseconds TotalPlayed;
    uint_t Loops = 0;
  };

  class TrackStateIterator : public Iterator
  {
  public:
    TrackStateIterator(Time::Microseconds frameDuration, TrackModel::Ptr model)
      : Model(std::move(model))
      , Cursor(frameDuration, *Model)
    {}

    // iterator functions
    void Reset() override
    {
      Cursor.Reset();
    }

    void NextFrame() override
    {
      if (!Cursor.NextFrame())
      {
        MoveToLoop();
      }
    }

    State GetState() const override
    {
      return Cursor.Get();
    }

  private:
    void MoveToLoop()
    {
      if (LoopState)
      {
        Cursor.SetState(*LoopState);
      }
      else
      {
        Cursor.Seek(Model->GetOrder().GetLoopPosition());
        LoopState = Cursor.GetState();
      }
      Cursor.DoneLoop();
    }

  private:
    const TrackModel::Ptr Model;
    TrackStateCursor Cursor;
    std::optional<PlainTrackState> LoopState;
  };

  Information CreateTrackInfoFixedChannels(Time::Microseconds frameDuration, const TrackModel& model, uint_t channels)
  {
    const auto& order = model.GetOrder();
    TrackLayout track = {
        .ChannelsCount = channels, .PositionsCount = order.GetSize(), .LoopPosition = order.GetLoopPosition()};
    TrackStateCursor cursor(frameDuration, model);
    cursor.Seek(track.LoopPosition);
    const auto loopAt = cursor.At();
    cursor.Seek(track.PositionsCount);
    const auto endAt = cursor.At();
    return {.Duration = endAt - Time::AtMillisecond(), .LoopDuration = endAt - loopAt, .Track = std::move(track)};
  }

  Iterator::Ptr CreateTrackStateIterator(Time::Microseconds frameDuration, TrackModel::Ptr model)
  {
    return MakePtr<TrackStateIterator>(frameDuration, std::move(model));
  }
}  // namespace Module
