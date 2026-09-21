/**
 *
 * @file
 *
 * @brief  Interfaces for multitrack chiptunes with undivideable tracks (e.g. SID)
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "formats/chiptune/container.h"

#include "types.h"

#include <memory>

namespace Formats::Multitrack
{
  class Container : public Chiptune::Container
  {
  public:
    using Ptr = std::shared_ptr<const Container>;

    //! @return total tracks count
    virtual uint_t TracksCount() const = 0;

    //! @return 0-based index of first track
    virtual uint_t StartTrackIndex() const = 0;
  };
}  // namespace Formats::Multitrack
