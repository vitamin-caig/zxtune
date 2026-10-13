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

#include <algorithm>

namespace Module
{
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
