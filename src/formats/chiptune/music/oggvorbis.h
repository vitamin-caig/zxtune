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

//local includes
#include "formats/chiptune/builder_meta.h"
//library includes
#include <formats/chiptune.h>

namespace Formats
{
  namespace Chiptune
  {
    namespace OggVorbis
    {
      //Use simplified parsing due to thirdparty library used
      class Builder
      {
      public:
        virtual ~Builder() = default;

        virtual MetaBuilder& GetMetaBuilder() = 0;
        
        virtual void SetChannels(uint_t channels) = 0;
        virtual void SetFrequency(uint_t frequency) = 0;
        virtual void AddFrame(std::size_t offset, uint_t framesCount) = 0;
      };

      Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
      Builder& GetStubBuilder();
    }

    Decoder::Ptr CreateOGGDecoder();
  }
}
