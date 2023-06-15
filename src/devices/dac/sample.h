/**
 *
 * @file
 *
 * @brief  DAC sample interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// library includes
#include <sound/sample.h>
// std includes
#include <memory>

namespace Devices::DAC
{
  class Sample
  {
  public:
    using Ptr = std::shared_ptr<const Sample>;
    virtual ~Sample() = default;

    virtual Sound::Sample::Type Get(std::size_t pos) const = 0;
    virtual std::size_t Size() const = 0;
    virtual std::size_t Loop() const = 0;
  };
}  // namespace Devices::DAC
