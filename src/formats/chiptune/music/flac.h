/**
 *
 * @file
 *
 * @brief  FLAC parser interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// local includes
#include "formats/chiptune/builder_meta.h"
// library includes
#include <formats/chiptune.h>

namespace Formats::Chiptune
{
  namespace Flac
  {
    // Use simplified parsing due to thirdparty library used
    class Builder
    {
    public:
      virtual ~Builder() = default;

      virtual MetaBuilder& GetMetaBuilder() = 0;

      virtual void SetBlockSize(uint_t minimal, uint_t maximal) = 0;
      virtual void SetFrameSize(uint_t minimal, uint_t maximal) = 0;
      virtual void SetStreamParameters(uint_t sampleRate, uint_t channels, uint_t bitsPerSample) = 0;
      virtual void SetTotalSamples(uint64_t count) = 0;

      virtual void AddFrame(std::size_t offset) = 0;
    };

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
    Builder& GetStubBuilder();
  }  // namespace Flac

  Decoder::Ptr CreateFLACDecoder();
}  // namespace Formats::Chiptune
