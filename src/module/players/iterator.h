/**
 *
 * @file
 *
 * @brief  Iterators interfaces
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/information.h"
#include "module/state.h"

#include <memory>

namespace Module
{
  //! @brief Frame-domain module information, converted to time on demand
  struct FramedInformation
  {
    //! Total frames count
    uint_t Total = 0;
    //! Loop frame index
    uint_t Loop = 0;
    //! Layout for track-structured modules
    std::optional<TrackLayout> Track = {};
  };

  //! @brief Frame-domain module state, converted to time on demand
  struct FramedState
  {
    //! Current frame
    uint_t At = 0;
    //! Count of restarts due to looping
    uint_t LoopCount = 0;
    //! Optional track state
    std::optional<TrackState> Track = {};
  };

  Information ToInformation(const FramedInformation& framed, Time::Microseconds frameDuration);
  State ToState(const FramedState& framed, Time::Microseconds frameDuration);

  //! Forward-only endlessly looping stream of per-frame device data
  template<class Data>
  class Iterator
  {
  public:
    using Ptr = std::unique_ptr<Iterator>;

    virtual ~Iterator() = default;

    //! State of the frame the next Advance() will yield
    virtual FramedState GetState() const = 0;
    //! Produces the current frame (advancing internal state) and steps to the next one
    virtual Data Advance() = 0;
    //! Returns to the initial frame
    virtual void Reset() = 0;
  };
}  // namespace Module
