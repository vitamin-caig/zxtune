/**
 *
 * @file
 *
 * @brief  TFM-based chiptunes support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/players/iterator.h"
#include "module/players/track_model.h"

#include "devices/tfm.h"
#include "module/information.h"
#include "module/track_presentation.h"
#include "parameters/accessor.h"

#include <memory>

namespace Module::TFM
{
  const auto BASE_FRAME_DURATION = Time::Microseconds::FromFrequency(50);

  using DataIterator = Iterator<Devices::TFM::Registers>;

  class Chiptune
  {
  public:
    using Ptr = std::unique_ptr<const Chiptune>;
    virtual ~Chiptune() = default;

    virtual Time::Microseconds GetFrameDuration() const = 0;

    virtual FramedInformation GetInformation() const = 0;
    virtual DataIterator::Ptr CreateDataIterator() const = 0;
  };

  class TrackingChiptune : public Chiptune
  {
  public:
    virtual const TrackModel& GetTrackModel() const = 0;
    virtual std::unique_ptr<const TrackPresentation> CreateTrackPresentation() const = 0;
  };
}  // namespace Module::TFM
