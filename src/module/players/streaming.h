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
#include "module/state.h"  // IWYU pragma: export
#include "time/duration.h"
#include "time/instant.h"

#include "types.h"

#include <memory>

namespace Module
{
  class StreamModel;

  Information::Ptr CreateStreamInfo(Time::Microseconds frameDuration, const StreamModel& model);
  StateIterator::Ptr CreateStreamStateIterator(Time::Microseconds frameDuration, const StreamModel& model);

  Information::Ptr CreateTimedInfo(Time::Milliseconds duration);
  Information::Ptr CreateTimedInfo(Time::Milliseconds duration, Time::Milliseconds loopDuration);

  class TimedState : public Module::State
  {
  public:
    using Ptr = std::shared_ptr<TimedState>;

    explicit TimedState(Time::Microseconds duration)
      : Limit(Time::AtMicrosecond() + duration)
    {}

    Time::AtMillisecond At() const override
    {
      return Position.CastTo<Time::Millisecond>();
    }

    Time::Milliseconds Total() const override
    {
      return TotalPlayback.CastTo<Time::Millisecond>();
    }

    uint_t LoopCount() const override
    {
      return Loops;
    }

    void Reset()
    {
      Position = {};
      Loops = 0;
      TotalPlayback = {};
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
    Time::Microseconds TotalPlayback;
    uint_t Loops = 0;
  };

  Information::Ptr CreateSampledInfo(uint_t samplerate, uint64_t totalSamples);

  class SampledState : public Module::State
  {
  public:
    using Ptr = std::shared_ptr<SampledState>;

    SampledState(uint64_t totalSamples, uint_t samplerate)
      : TotalSamples(totalSamples)
      , Samplerate(samplerate)
    {}

    Time::AtMillisecond At() const override
    {
      return Time::AtMillisecond() + Time::Milliseconds::FromRatio(DoneSamples, Samplerate);
    }

    Time::Milliseconds Total() const override
    {
      return Time::Milliseconds::FromRatio(DoneSamplesTotal, Samplerate);
    }

    uint_t LoopCount() const override
    {
      return Loops;
    }

    void Reset()
    {
      DoneSamples = DoneSamplesTotal = 0;
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
    uint64_t DoneSamplesTotal = 0;
    uint_t Loops = 0;
  };
}  // namespace Module
