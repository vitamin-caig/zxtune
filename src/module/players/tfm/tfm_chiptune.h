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

// library includes
#include <devices/tfm.h>
#include <module/information.h>
#include <module/players/iterator.h>
#include <parameters/accessor.h>

namespace Module::TFM
{
  const auto BASE_FRAME_DURATION = Time::Microseconds::FromFrequency(50);

  class DataIterator : public Iterator
  {
  public:
    using Ptr = std::shared_ptr<DataIterator>;

    virtual State::Ptr GetStateObserver() const = 0;

    virtual void GetData(Devices::TFM::Registers& res) const = 0;
  };

  class Chiptune
  {
  public:
    using Ptr = std::shared_ptr<const Chiptune>;
    virtual ~Chiptune() = default;

    virtual Time::Microseconds GetFrameDuration() const = 0;

    virtual Information::Ptr GetInformation() const = 0;
    virtual Parameters::Accessor::Ptr GetProperties() const = 0;
    virtual DataIterator::Ptr CreateDataIterator() const = 0;
  };
}  // namespace Module::TFM
