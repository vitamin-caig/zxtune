/**
 *
 * @file
 *
 * @brief  State iterators support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/iterator.h"

namespace Module
{
  Information ToInformation(const FramedInformation& framed, Time::Microseconds frameDuration)
  {
    return {.Duration = (frameDuration * framed.Total).CastTo<Time::Millisecond>(),
            .LoopDuration = (frameDuration * (framed.Total - framed.Loop)).CastTo<Time::Millisecond>(),
            .Track = framed.Track};
  }

  State ToState(const FramedState& framed, Time::Microseconds frameDuration)
  {
    return {.At = Time::AtMillisecond() + (frameDuration * framed.At).CastTo<Time::Millisecond>(),
            .LoopCount = framed.LoopCount,
            .Track = framed.Track};
  }
}  // namespace Module
