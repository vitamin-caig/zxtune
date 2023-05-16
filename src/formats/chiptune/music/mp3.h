/**
 *
 * @file
 *
 * @brief  MP3 parser interface
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
  namespace Mp3
  {
    struct Frame
    {
      struct DataLocation
      {
        std::size_t Offset = 0;
        std::size_t Size = 0;
      };

      struct SoundProperties
      {
        uint_t Samplerate = 0;
        uint_t SamplesCount = 0;
        bool Mono = false;
      };

      DataLocation Location;
      SoundProperties Properties;
    };

    // Use simplified parsing due to thirdparty library used
    class Builder
    {
    public:
      virtual ~Builder() = default;

      virtual MetaBuilder& GetMetaBuilder() = 0;

      virtual void AddFrame(const Frame& frame) = 0;
    };

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
    Builder& GetStubBuilder();
  }  // namespace Mp3

  Decoder::Ptr CreateMP3Decoder();
}  // namespace Formats::Chiptune
