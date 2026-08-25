/**
 *
 * @file
 *
 * @brief  Chiptune container interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "binary/container.h"

#include "types.h"

#include <memory>

namespace Formats::Chiptune
{
  //! @brief Chiptune raw data presentation
  class Container : public Binary::Container
  {
  public:
    using Ptr = std::shared_ptr<const Container>;

    virtual uint_t Checksum() const = 0;

    //! @brief Internal structures simple fingerprint
    //! @return Some integer value at least 32-bit
    virtual uint_t FixedChecksum() const = 0;
  };
}  // namespace Formats::Chiptune
