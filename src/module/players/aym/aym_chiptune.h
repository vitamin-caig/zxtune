/**
 *
 * @file
 *
 * @brief  AYM-based chiptunes support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// local includes
#include "module/players/aym/aym_parameters.h"
#include "module/players/stream_model.h"
#include "module/players/track_model.h"
// library includes
#include <devices/aym.h>
#include <module/players/iterator.h>
#include <module/players/streaming.h>
#include <module/players/tracking.h>

namespace Module::AYM
{
  const auto BASE_FRAME_DURATION = Time::Microseconds::FromFrequency(50);

  class DataIterator : public Iterator
  {
  public:
    using Ptr = std::shared_ptr<DataIterator>;

    virtual State::Ptr GetStateObserver() const = 0;

    virtual Devices::AYM::Registers GetData() const = 0;
  };

  class Chiptune
  {
  public:
    using Ptr = std::shared_ptr<const Chiptune>;
    virtual ~Chiptune() = default;

    virtual Time::Microseconds GetFrameDuration() const = 0;

    // One of
    virtual TrackModel::Ptr FindTrackModel() const = 0;
    virtual Module::StreamModel::Ptr FindStreamModel() const = 0;

    virtual Parameters::Accessor::Ptr GetProperties() const = 0;
    virtual DataIterator::Ptr CreateDataIterator(TrackParameters::Ptr trackParams) const = 0;
  };
}  // namespace Module::AYM
