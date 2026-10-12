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

#include "make_ptr.h"

#include <algorithm>
#include <utility>

namespace Module
{
  struct FramedStream
  {
    uint_t TotalFrames = 0;
    uint_t LoopFrame = 0;

    void Sanitize()
    {
      if (LoopFrame >= TotalFrames)
      {
        LoopFrame = 0;
      }
    }
  };

  class FramedStreamStateCursor
  {
  public:
    explicit FramedStreamStateCursor(FramedStream stream)
      : Stream(stream)
    {
      Reset();
    }

    FramedState Get() const
    {
      return {.At = CurFrame, .LoopCount = Loops};
    }

    // navigation
    bool IsValid() const
    {
      return CurFrame < Stream.TotalFrames;
    }

    void Reset()
    {
      CurFrame = 0;
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
        ++CurFrame;
      }
    }

  private:
    const FramedStream Stream;
    uint_t CurFrame = 0;
    uint_t Loops = 0;
  };

  class FramedStreamStateIterator : public Iterator
  {
  public:
    explicit FramedStreamStateIterator(FramedStream stream)
      : Cursor(stream)
    {}

    // iterator functions
    void Reset() override
    {
      Cursor.Reset();
    }

    void NextFrame() override
    {
      Cursor.NextFrame();
      if (!Cursor.IsValid())
      {
        Cursor.ResetToLoop();
      }
    }

    FramedState GetState() const override
    {
      return Cursor.Get();
    }

  private:
    FramedStreamStateCursor Cursor;
  };

  Iterator::Ptr CreateStreamStateIterator(const FramedInformation& info)
  {
    FramedStream stream;
    stream.TotalFrames = info.Total;
    stream.LoopFrame = info.Loop;
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
    const auto doneLoops = nextSamples / TotalSamples;
    Loops += doneLoops;
    return doneLoops;
  }
}  // namespace Module
