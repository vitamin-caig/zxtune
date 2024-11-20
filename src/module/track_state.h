/**
 *
 * @file
 *
 * @brief  TrackState interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/state.h"

namespace Module
{
  //! @brief Runtime module track status
  class TrackState : public State
  {
  public:
    //! Pointer type
    using Ptr = std::shared_ptr<const TrackState>;

    //! Current position (up to TrackInformation::PositionsCount)
    virtual uint_t Position() const = 0;
    //! Current pattern
    virtual uint_t Pattern() const = 0;
    //! Current line in pattern
    virtual uint_t Line() const = 0;
    //! Current tempo
    virtual uint_t Tempo() const = 0;
    //! Current quirk in line
    virtual uint_t Quirk() const = 0;
    //! Current active channels count (up to Information::Channels)
    virtual uint_t Channels() const = 0;
  };
}  // namespace Module
