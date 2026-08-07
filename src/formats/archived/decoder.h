/**
 *
 * @file
 *
 * @brief  Archived data detector interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "binary/format.h"
#include "formats/archived/container.h"

#include "string_view.h"

#include <memory>

namespace Formats::Archived
{
  //! @brief Decoding functionality provider
  class Decoder
  {
  public:
    using Ptr = std::unique_ptr<const Decoder>;
    virtual ~Decoder() = default;

    //! @brief Get short decoder description
    virtual StringView GetDescription() const = 0;

    //! @brief Get approximate format description to search in raw binary data
    //! @invariant Cannot be empty
    virtual Binary::Format::Ptr GetFormat() const = 0;

    //! @brief Perform raw data decoding
    //! @param rawData Data to be decoded
    //! @return Non-null object if data is successfully recognized and decoded
    //! @invariant Result is always rawData's subcontainer
    virtual Container::Ptr Decode(const Binary::Container& rawData) const = 0;
  };
}  // namespace Formats::Archived
