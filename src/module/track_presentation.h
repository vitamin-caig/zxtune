/**
 *
 * @file
 *
 * @brief  Track pattern presentation interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "string_type.h"
#include "types.h"

#include <memory>

namespace Module
{
  //! @brief Optional module holder capability to render pattern content as text
  class TrackPresentation
  {
  public:
    //! @brief Pointer type
    using Ptr = std::shared_ptr<const TrackPresentation>;

    virtual ~TrackPresentation() = default;

    //! @brief Ready-to-show text for one pattern, one line per row, '\n'-separated, no trailing newline
    //! @return empty string for invalid pattern index or when no track model is available
    virtual String GetPattern(uint_t index) const = 0;
  };
}  // namespace Module
