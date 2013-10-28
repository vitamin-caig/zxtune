/*
Abstract:
  TurboSound containers format description

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef FORMATS_CHIPTUNE_TURBOSOUND_H_DEFINED
#define FORMATS_CHIPTUNE_TURBOSOUND_H_DEFINED

//common includes
#include <types.h>
//library includes
#include <formats/chiptune.h>

namespace Formats
{
  namespace Chiptune
  {
    namespace TurboSound
    {
      class Builder
      {
      public:
        typedef boost::shared_ptr<Builder> Ptr;
        virtual ~Builder() {}

        virtual void SetFirstSubmoduleLocation(std::size_t offset, std::size_t size) = 0;
        virtual void SetSecondSubmoduleLocation(std::size_t offset, std::size_t size) = 0;
      };

      Builder& GetStubBuilder();

      class Decoder : public Formats::Chiptune::Decoder
      {
      public:
        typedef boost::shared_ptr<const Decoder> Ptr;

        virtual Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target) const = 0;
      };

      Decoder::Ptr CreateDecoder();
    }
  }
}

#endif //FORMATS_CHIPTUNE_TURBOSOUND_H_DEFINED
