/**
 *
 * @file
 *
 * @brief  Streamed modules support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "module/players/streaming.h"
// common includes
#include <make_ptr.h>
// std includes
#include <utility>

namespace Module
{
  struct FramedStream
  {
    uint_t TotalFrames = 0;
    uint_t LoopFrame = 0;
    Time::Microseconds FrameDuration;
  };

  class FramedStreamStateCursor : public State
  {
  public:
    using Ptr = std::shared_ptr<FramedStreamStateCursor>;

    explicit FramedStreamStateCursor(FramedStream stream)
      : Stream(stream)
      , CurFrame()
      , Loops()
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
    uint_t CurFrame;
    Time::Microseconds TotalPlayed;
    uint_t Loops;
  };

  class FramedStreamInfo : public Information
  {
  public:
    FramedStreamInfo(FramedStream stream)
      : Stream(stream)
    {}

    Time::Milliseconds Duration() const override
    {
      return (Stream.FrameDuration * Stream.TotalFrames).CastTo<Time::Millisecond>();
    }

    Time::Milliseconds LoopDuration() const override
    {
      return (Stream.FrameDuration * (Stream.TotalFrames - Stream.LoopFrame)).CastTo<Time::Millisecond>();
    }

  private:
    const FramedStream Stream;
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

  Information::Ptr CreateStreamInfo(Time::Microseconds frameDuration, const StreamModel::Ptr& model)
  {
    FramedStream stream;
    stream.FrameDuration = frameDuration;
    stream.TotalFrames = model->GetTotalFrames();
    stream.LoopFrame = model->GetLoopFrame();
    return MakePtr<FramedStreamInfo>(stream);
  }

  StateIterator::Ptr CreateStreamStateIterator(Time::Microseconds frameDuration, const StreamModel::Ptr& model)
  {
    FramedStream stream;
    stream.FrameDuration = frameDuration;
    stream.TotalFrames = model->GetTotalFrames();
    stream.LoopFrame = model->GetLoopFrame();
    return MakePtr<FramedStreamStateIterator>(stream);
  }

  class TimedInfo : public Module::Information
  {
  public:
    TimedInfo(Time::Milliseconds duration, Time::Milliseconds loopDuration)
      : DurationValue(duration)
      , LoopDurationValue(loopDuration)
    {}

    Time::Milliseconds Duration() const override
    {
      return DurationValue;
    }

    Time::Milliseconds LoopDuration() const override
    {
      return LoopDurationValue;
    }

  private:
    const Time::Milliseconds DurationValue;
    const Time::Milliseconds LoopDurationValue;
  };

  Information::Ptr CreateTimedInfo(Time::Milliseconds duration)
  {
    return MakePtr<TimedInfo>(duration, duration);
  }

  Information::Ptr CreateTimedInfo(Time::Milliseconds duration, Time::Milliseconds loopDuration)
  {
    return MakePtr<TimedInfo>(duration, loopDuration);
  }

  Time::Microseconds TimedState::Consume(Time::Microseconds range)
  {
    const auto nextPos = range.Get() ? Position + range : Limit;
    Position = Time::AtMicrosecond(nextPos.Get() % Limit.Get());
    Loops += nextPos.Get() / Limit.Get();
    TotalPlayback += range;
    return range;
  }

  Information::Ptr CreateSampledInfo(uint_t samplerate, uint64_t totalSamples)
  {
    return CreateTimedInfo(Time::Milliseconds::FromRatio(totalSamples, samplerate));
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
