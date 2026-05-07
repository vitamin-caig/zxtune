/**
 *
 * @file
 *
 * @brief  Streamed modules support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/streaming.h"

#include "module/players/stream_model.h"

#include "make_ptr.h"

#include <algorithm>
#include <utility>

namespace Module
{
  struct FramedStream
  {
    uint_t TotalFrames = 0;
    uint_t LoopFrame = 0;
    Time::Microseconds FrameDuration;

    void Sanitize()
    {
      if (LoopFrame >= TotalFrames)
      {
        LoopFrame = 0;
      }
    }
  };

  class FramedStreamStateCursor : public State
  {
  public:
    using Ptr = std::shared_ptr<FramedStreamStateCursor>;

    explicit FramedStreamStateCursor(FramedStream stream)
      : Stream(stream)
    {
      Reset();
    }

    // status functions
    Time::AtMillisecond At() const override
    {
      return Time::AtMillisecond() + (Stream.FrameDuration * CurFrame).CastTo<Time::Millisecond>();
    }

    Time::Milliseconds Total() const override
    {
      return TotalPlayed.CastTo<Time::Millisecond>();
    }

    uint_t LoopCount() const override
    {
      return Loops;
    }

    uint_t Frame() const
    {
      return CurFrame;
    }

    // navigation
    bool IsValid() const
    {
      return CurFrame < Stream.TotalFrames;
    }

    void Reset()
    {
      CurFrame = 0;
      TotalPlayed = {};
      Loops = 0;
    }

    void ResetToLoop()
    {
      CurFrame = Stream.LoopFrame;
      ++Loops;
    }

    void NextFrame()
    {
      if (IsValid())
      {
        TotalPlayed += Stream.FrameDuration;
        ++CurFrame;
      }
    }

  private:
    const FramedStream Stream;
    uint_t CurFrame = 0;
    Time::Microseconds TotalPlayed;
    uint_t Loops = 0;
  };

  class FramedStreamStateIterator : public StateIterator
  {
  public:
    explicit FramedStreamStateIterator(FramedStream stream)
      : Cursor(MakePtr<FramedStreamStateCursor>(stream))
    {}

    // iterator functions
    void Reset() override
    {
      Cursor->Reset();
    }

    void NextFrame() override
    {
      Cursor->NextFrame();
      if (!Cursor->IsValid())
      {
        Cursor->ResetToLoop();
      }
    }

    uint_t CurrentFrame() const override
    {
      return Cursor->Frame();
    }

    State::Ptr GetStateObserver() const override
    {
      return Cursor;
    }

  private:
    const FramedStreamStateCursor::Ptr Cursor;
  };

  Information CreateStreamInfo(Time::Microseconds frameDuration, const StreamModel& model)
  {
    FramedStream stream;
    stream.FrameDuration = frameDuration;
    stream.TotalFrames = model.GetTotalFrames();
    stream.LoopFrame = model.GetLoopFrame();
    stream.Sanitize();
    return CreateTimedInfo(
        (stream.FrameDuration * stream.TotalFrames).CastTo<Time::Millisecond>(),
        (stream.FrameDuration * (stream.TotalFrames - stream.LoopFrame)).CastTo<Time::Millisecond>());
  }

  StateIterator::Ptr CreateStreamStateIterator(Time::Microseconds frameDuration, const StreamModel& model)
  {
    FramedStream stream;
    stream.FrameDuration = frameDuration;
    stream.TotalFrames = model.GetTotalFrames();
    stream.LoopFrame = model.GetLoopFrame();
    stream.Sanitize();
    return MakePtr<FramedStreamStateIterator>(stream);
  }

  template<class Unit>
  auto Modulo(Time::Instant<Unit> pos, Time::Instant<Unit> limit)
  {
    return std::make_pair(Time::Instant<Unit>(pos.Get() % limit.Get()), pos.Get() / limit.Get());
  }

  Time::Microseconds TimedState::ConsumeUpTo(Time::Microseconds range)
  {
    const Time::Microseconds avail = Limit - Position;
    const auto toConsume = std::min(range, avail);
    const auto delta = Modulo(Position + toConsume, Limit);
    Position = delta.first;
    Loops += delta.second;
    return toConsume;
  }

  Time::Microseconds TimedState::ConsumeRest()
  {
    return ConsumeUpTo(Limit - Position);
  }

  uint_t SampledState::Consume(uint_t samples)
  {
    const auto nextSamples = samples ? DoneSamples + samples : TotalSamples;
    DoneSamples = nextSamples % TotalSamples;
    DoneSamplesTotal += samples;
    const auto doneLoops = nextSamples / TotalSamples;
    Loops += doneLoops;
    return doneLoops;
  }
}  // namespace Module
