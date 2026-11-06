/**
 *
 * @file
 *
 * @brief  DAC-based chiptune interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/players/generic_presentation.h"
#include "module/players/iterator.h"
#include "module/players/track_model.h"

#include "devices/dac.h"
#include "parameters/accessor.h"

namespace Module::DAC
{
  const auto BASE_FRAME_DURATION = Time::Microseconds::FromFrequency(50);

  using DataIterator = Iterator<Devices::DAC::Channels>;

  class Chiptune
  {
  public:
    using Ptr = std::unique_ptr<const Chiptune>;
    using PresentPatternFunction = Presentation::Generic::PresentPatternFunction;

    virtual ~Chiptune() = default;

    static Time::Microseconds GetFrameDuration()
    {
      return BASE_FRAME_DURATION;
    }

    virtual TrackModel::Ptr GetTrackModel() const = 0;
    virtual DataIterator::Ptr CreateDataIterator() const = 0;
    virtual void GetSamples(Devices::DAC::Chip& chip) const = 0;
    //! @brief Per-format pattern renderer; nullptr means the generic layout
    virtual PresentPatternFunction GetPresentPattern() const
    {
      return nullptr;
    }
  };
}  // namespace Module::DAC
