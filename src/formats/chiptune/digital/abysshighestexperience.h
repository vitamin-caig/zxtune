/**
* 
* @file
*
* @brief  Abyss' Highest Experience support interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "formats/chiptune/builder_meta.h"
#include "formats/chiptune/builder_pattern.h"
//library includes
#include <formats/chiptune.h>

namespace Formats
{
  namespace Chiptune
  {
    namespace AbyssHighestExperience
    {
      //Use simplified parsing due to thirdparty library used
      class Builder
      {
      public:
        virtual ~Builder() = default;

        virtual MetaBuilder& GetMetaBuilder() = 0;
      };

      Builder& GetStubBuilder();

      class Decoder : public Formats::Chiptune::Decoder
      {
      public:
        typedef std::shared_ptr<const Decoder> Ptr;

        virtual Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target) const = 0;
      };

      Decoder::Ptr CreateDecoder();
    }
    
    Decoder::Ptr CreateAbyssHighestExperienceDecoder();
  }
}
