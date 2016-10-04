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

//common includes
#include <types.h>
//std includes
#include <memory>

namespace Module
{
  //! @brief Runtime module track status
  class TrackState
  {
  public:
    //! Pointer type
    typedef std::shared_ptr<const TrackState> Ptr;

    virtual ~TrackState() {}

    //! Current position (up to Information::PositionsCount)
    virtual uint_t Position() const = 0;
    //! Current pattern (up to Information::PatternsCount)
    virtual uint_t Pattern() const = 0;
    //! Current pattern size
    virtual uint_t PatternSize() const = 0;
    //! Current line in pattern (up to Status::PatternSize)
    virtual uint_t Line() const = 0;
    //! Current tempo
    virtual uint_t Tempo() const = 0;
    //! Current quirk in line (up to Status::Tempo)
    virtual uint_t Quirk() const = 0;
    //! Current frame in track (up to Information::FramesCount)
    virtual uint_t Frame() const = 0;
    //! Current active channels count (up to Information::Channels)
    virtual uint_t Channels() const = 0;
  };
}
