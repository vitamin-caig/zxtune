/**
 *
 * @file
 *
 * @brief  Generic text presentation of track models
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/track_presentation.h"

namespace Module
{
  class TrackModel;

  //! @brief Tracker-like text rendering of a TrackModel pattern
  class GenericTrackPresentation : public TrackPresentation
  {
  public:
    explicit GenericTrackPresentation(const TrackModel& model);

    std::string GetPattern(std::size_t index) const override;

  private:
    const TrackModel& Model;
  };
}  // namespace Module
