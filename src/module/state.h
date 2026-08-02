/**
 *
 * @file
 *
 * @brief  State interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "time/duration.h"
#include "time/instant.h"

#include "types.h"

#include <optional>

namespace Module
{
  //! @brief Runtime module track status
  struct TrackState
  {
    //! Current position (up to TrackInformation::PositionsCount)
    uint_t Position = 0;
    //! Current pattern
    uint_t Pattern = 0;
    //! Current line in pattern
    uint_t Line = 0;
    //! Current tempo
    uint_t Tempo = 0;
    //! Current quirk in line
    uint_t Quirk = 0;
    //! Current active channels count (up to Information::Channels)
    uint_t Channels = 0;
  };

  //! @brief Runtime module status
  struct State
  {
    //! Current playback position till Information::Duration
    Time::AtMillisecond At;
    //! Total played time ignoring seeks
    Time::Milliseconds Total;
    //! Count of restarts due to looping
    uint_t LoopCount = 0;
    //! Optional track state
    std::optional<TrackState> Track = {};
  };
}  // namespace Module
