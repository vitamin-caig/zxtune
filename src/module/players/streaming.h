/**
 *
 * @file
 *
 * @brief  Streamed modules support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/players/iterator.h"

#include "module/information.h"
#include "time/duration.h"

#include <utility>

namespace Module
{
  //! Frame-domain cursor for stream (lookup) modules
  class StreamCursor
  {
  public:
    explicit StreamCursor(const FramedInformation& info)
      : TotalFrames(info.Total)
      , LoopFrame(info.Loop < info.Total ? info.Loop : 0)
    {}

    FramedState Get() const
    {
      return {.At = CurFrame, .LoopCount = Loops};
    }

    uint_t At() const
    {
      return CurFrame;
    }

    void Reset()
    {
      CurFrame = 0;
      Loops = 0;
    }

    void NextFrameLooped()
    {
      if (++CurFrame >= TotalFrames)
      {
        CurFrame = LoopFrame;
        ++Loops;
      }
    }

  private:
    const uint_t TotalFrames;
    const uint_t LoopFrame;
    uint_t CurFrame = 0;
    uint_t Loops = 0;
  };

  //! Data source for stream modules which precompute per-frame device data
  template<class ModelType>
  class StreamIterator : public Iterator<typename ModelType::Data>
  {
  public:
    explicit StreamIterator(typename ModelType::Ptr model)
      : Model(std::move(model))
      , Cursor(Model->GetInformation())
    {}

    FramedState GetState() const override
    {
      return Cursor.Get();
    }

    typename ModelType::Data Advance() override
    {
      auto data = Model->Get(Cursor.At());
      Cursor.NextFrameLooped();
      return data;
    }

    void Reset() override
    {
      Cursor.Reset();
    }

  private:
    const typename ModelType::Ptr Model;
    StreamCursor Cursor;
  };

  inline Information CreateTimedInfo(Time::Milliseconds duration, Time::Milliseconds loopDuration)
  {
    return {.Duration = duration, .LoopDuration = loopDuration};
  }

  inline Information CreateTimedInfo(Time::Milliseconds duration)
  {
    return CreateTimedInfo(duration, duration);
  }

  class TimedState
  {
  public:
    explicit TimedState(Time::Microseconds duration)
      : Limit(Time::AtMicrosecond() + duration)
    {}

    Time::AtMillisecond At() const
    {
      return Position.CastTo<Time::Millisecond>();
    }

    uint_t LoopCount() const
    {
      return Loops;
    }

    State Get() const
    {
      return {.At = At(), .LoopCount = LoopCount()};
    }

    void Reset()
    {
      Position = {};
      Loops = 0;
    }

    // Consumes and returns min(range, End - Position)
    Time::Microseconds ConsumeUpTo(Time::Microseconds range);
    Time::Microseconds ConsumeRest();

    // Returns delta to skip
    Time::Microseconds Seek(Time::AtMicrosecond request)
    {
      Loops = 0;
      if (request < Position)
      {
        Position = request;
        return Time::Microseconds{request.Get()};
      }
      else
      {
        const auto newPos = Time::AtMicrosecond(request.Get() % Limit.Get());
        const auto delta = newPos - Position;
        Position = newPos;
        return delta;
      }
    }

    Time::AtMicrosecond PreciseAt() const
    {
      return Position;
    }

  private:
    const Time::AtMicrosecond Limit;
    Time::AtMicrosecond Position;
    uint_t Loops = 0;
  };

  inline Information CreateSampledInfo(uint_t samplerate, uint64_t totalSamples)
  {
    return CreateTimedInfo(Time::Milliseconds::FromRatio(totalSamples, samplerate));
  }

  class SampledState
  {
  public:
    SampledState(uint64_t totalSamples, uint_t samplerate)
      : TotalSamples(totalSamples)
      , Samplerate(samplerate)
    {}

    Time::AtMillisecond At() const
    {
      return Time::AtMillisecond() + Time::Milliseconds::FromRatio(DoneSamples, Samplerate);
    }

    State Get() const
    {
      return {.At = At(), .LoopCount = Loops};
    }

    void Reset()
    {
      DoneSamples = 0;
      Loops = 0;
    }

    // Set samples = 0 to enforce stream end
    // return done loops count
    uint_t Consume(uint_t samples);

    void Seek(Time::AtMillisecond request)
    {
      SeekAtSample(uint64_t(Samplerate) * request.Get() / request.PER_SECOND);
    }

    void SeekAtSample(uint64_t samples)
    {
      DoneSamples = samples % TotalSamples;
    }

    uint64_t AtSample() const
    {
      return DoneSamples;
    }

  private:
    const uint64_t TotalSamples;
    const uint_t Samplerate;
    uint64_t DoneSamples = 0;
    uint_t Loops = 0;
  };
}  // namespace Module
