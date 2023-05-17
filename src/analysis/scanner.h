/**
 *
 * @file
 *
 * @brief Scanner interface and factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// library includes
#include <binary/container.h>
#include <formats/archived.h>
#include <formats/chiptune.h>
#include <formats/image.h>
#include <formats/packed.h>

namespace Analysis
{
  class Scanner
  {
  public:
    using Ptr = std::shared_ptr<const Scanner>;
    using RWPtr = std::shared_ptr<Scanner>;

    virtual ~Scanner() = default;

    virtual void AddDecoder(Formats::Archived::Decoder::Ptr decoder) = 0;
    virtual void AddDecoder(Formats::Packed::Decoder::Ptr decoder) = 0;
    virtual void AddDecoder(Formats::Image::Decoder::Ptr decoder) = 0;
    virtual void AddDecoder(Formats::Chiptune::Decoder::Ptr decoder) = 0;

    class Target
    {
    public:
      virtual ~Target() = default;

      virtual void Apply(const Formats::Archived::Decoder& decoder, std::size_t offset,
                         Formats::Archived::Container::Ptr data) = 0;
      virtual void Apply(const Formats::Packed::Decoder& decoder, std::size_t offset,
                         Formats::Packed::Container::Ptr data) = 0;
      virtual void Apply(const Formats::Image::Decoder& decoder, std::size_t offset,
                         Formats::Image::Container::Ptr data) = 0;
      virtual void Apply(const Formats::Chiptune::Decoder& decoder, std::size_t offset,
                         Formats::Chiptune::Container::Ptr data) = 0;
      virtual void Apply(std::size_t offset, Binary::Container::Ptr data) = 0;
    };

    virtual void Scan(Binary::Container::Ptr source, Target& target) const = 0;
  };

  Scanner::RWPtr CreateScanner();
}  // namespace Analysis
