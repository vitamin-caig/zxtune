/**
* 
* @file
*
* @brief  AYC support interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//common includes
#include <types.h>
//library includes
#include <formats/chiptune.h>

namespace Formats
{
  namespace Chiptune
  {
    namespace AYC
    {
      class Builder
      {
      public:
        typedef std::shared_ptr<Builder> Ptr;
        virtual ~Builder() {}

        virtual void SetFrames(std::size_t count) = 0;
        virtual void StartChannel(uint_t idx) = 0;
        virtual void AddValues(const Dump& values) = 0;
      };

      Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
      Builder& GetStubBuilder();
    }

    Decoder::Ptr CreateAYCDecoder();
  }
}
