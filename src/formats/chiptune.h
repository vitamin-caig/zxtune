/**
 *
 * @file
 *
 * @brief  Chiptunes support interfaces
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <types.h>
// library includes
#include <binary/container.h>
#include <binary/format.h>
// std includes
#include <memory>

namespace Formats::Chiptune
{
  //! @brief Chiptune raw data presentation
  class Container : public Binary::Container
  {
  public:
    using Ptr = std::shared_ptr<const Container>;

    //! @brief Internal structures simple fingerprint
    //! @return Some integer value at least 32-bit
    virtual uint_t FixedChecksum() const = 0;
  };

  //! @brief Decoding functionality provider
  class Decoder
  {
  public:
    using Ptr = std::shared_ptr<const Decoder>;
    virtual ~Decoder() = default;

    //! @brief Get short decoder description
    virtual String GetDescription() const = 0;

    //! @brief Get approximate format description to search in raw binary data
    //! @invariant Cannot be empty
    virtual Binary::Format::Ptr GetFormat() const = 0;

    //! @brief Fast check for data consistensy
    //! @param rawData Data to be checked
    //! @return false if rawData has defenitely wrong format, else otherwise
    virtual bool Check(Binary::View rawData) const = 0;
    //! @brief Perform raw data decoding
    //! @param rawData Data to be decoded
    //! @return Non-null object if data is successfully recognized and decoded
    //! @invariant Result is always rawData's subcontainer
    virtual Container::Ptr Decode(const Binary::Container& rawData) const = 0;
  };
}  // namespace Formats::Chiptune
