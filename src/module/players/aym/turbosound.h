/**
 *
 * @file
 *
 * @brief  TurboSound-based chiptunes support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// local includes
#include "module/players/aym/aym_base_track.h"
// library includes
#include <devices/turbosound.h>
#include <module/holder.h>

namespace Module::TurboSound
{
  const uint_t TRACK_CHANNELS = AYM::TRACK_CHANNELS * Devices::TurboSound::CHIPS;

  class DataIterator : public Iterator
  {
  public:
    using Ptr = std::unique_ptr<DataIterator>;

    virtual State::Ptr GetStateObserver() const = 0;

    virtual Devices::TurboSound::Registers GetData() const = 0;
  };

  class Chiptune
  {
  public:
    using Ptr = std::unique_ptr<const Chiptune>;
    virtual ~Chiptune() = default;

    virtual Time::Microseconds GetFrameDuration() const = 0;

    // One of
    virtual TrackModel::Ptr FindTrackModel() const = 0;
    virtual Module::StreamModel::Ptr FindStreamModel() const = 0;

    virtual Parameters::Accessor::Ptr GetProperties() const = 0;
    virtual DataIterator::Ptr CreateDataIterator(AYM::TrackParameters::Ptr first,
                                                 AYM::TrackParameters::Ptr second) const = 0;
  };

  Chiptune::Ptr CreateChiptune(Parameters::Accessor::Ptr params, AYM::Chiptune::Ptr first, AYM::Chiptune::Ptr second);

  Holder::Ptr CreateHolder(Chiptune::Ptr chiptune);
}  // namespace Module::TurboSound
