/**
* 
* @file
*
* @brief  WAV parser interface
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
    namespace Wav
    {
      //currently supported formats
      enum Format : uint_t
      {
        PCM = 1,
        IEEE_FLOAT = 3
      };
      
      class Builder
      {
      public:
        virtual ~Builder() = default;

        virtual MetaBuilder& GetMetaBuilder() = 0;
        
        virtual void SetProperties(uint_t format, uint_t frequency, uint_t channels, uint_t bits, uint_t blockSize) = 0;
        virtual void SetSamplesData(Binary::Container::Ptr data) = 0;
        virtual void SetSamplesCountHint(uint_t count) = 0;
      };

      Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
      Builder& GetStubBuilder();
    }

    Decoder::Ptr CreateWAVDecoder();
  }
}
