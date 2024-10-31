/**
 *
 * @file
 *
 * @brief  OGG vorbis parser interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "binary/view.h"
#include "formats/chiptune.h"

#include "types.h"

namespace Formats::Chiptune
{
  class MetaBuilder;

  namespace OggVorbis
  {
    // Use simplified parsing due to thirdparty library used
    class Builder
    {
    public:
      virtual ~Builder() = default;

      virtual MetaBuilder& GetMetaBuilder() = 0;

      virtual void SetStreamId(uint32_t id) = 0;
      virtual void AddUnknownPacket(Binary::View data) = 0;
      virtual void SetProperties(uint_t channels, uint_t frequency, uint_t blockSizeLo, uint_t blockSizeHi) = 0;
      // Full setup block, including header
      virtual void SetSetup(Binary::View data) = 0;
      virtual void AddFrame(std::size_t offset, uint64_t positionInFrames, Binary::View data) = 0;
    };

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
    Builder& GetStubBuilder();
  }  // namespace OggVorbis

  Decoder::Ptr CreateOGGDecoder();
}  // namespace Formats::Chiptune
