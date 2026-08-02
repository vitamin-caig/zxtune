/**
 *
 * @file
 *
 * @brief  Information interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "time/duration.h"

#include <optional>

namespace Module
{
  //! @brief Track module specific information
  struct TrackLayout
  {
    //! Channels count
    uint_t ChannelsCount = 0;
    //! Total positions
    uint_t PositionsCount = 0;
    //! Loop position index
    uint_t LoopPosition = 0;
  };

  //! @brief Common module information
  struct Information
  {
    //! Total module duration
    Time::Milliseconds Duration;
    //! Loop duration
    Time::Milliseconds LoopDuration;
    //! Layout for track-structured modules
    std::optional<TrackLayout> Track = {};
  };
}  // namespace Module
