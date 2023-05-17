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

// library includes
#include <devices/dac.h>
#include <module/players/iterator.h>
#include <module/players/track_model.h>
#include <parameters/accessor.h>

namespace Module::DAC
{
  const auto BASE_FRAME_DURATION = Time::Microseconds::FromFrequency(50);

  class DataIterator : public Iterator
  {
  public:
    using Ptr = std::shared_ptr<DataIterator>;

    virtual State::Ptr GetStateObserver() const = 0;

    virtual void GetData(Devices::DAC::Channels& data) const = 0;
  };

  class Chiptune
  {
  public:
    using Ptr = std::shared_ptr<const Chiptune>;
    virtual ~Chiptune() = default;

    Time::Microseconds GetFrameDuration() const
    {
      return BASE_FRAME_DURATION;
    }

    virtual TrackModel::Ptr GetTrackModel() const = 0;
    virtual Parameters::Accessor::Ptr GetProperties() const = 0;
    virtual DataIterator::Ptr CreateDataIterator() const = 0;
    virtual void GetSamples(Devices::DAC::Chip& chip) const = 0;
  };
}  // namespace Module::DAC
