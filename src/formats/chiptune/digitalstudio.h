/*
Abstract:
  DigitalStudio format description

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef FORMATS_CHIPTUNE_DIGITALSTUDIO_H_DEFINED
#define FORMATS_CHIPTUNE_DIGITALSTUDIO_H_DEFINED

//common includes
#include <binary/container.h>
//library includes
#include <formats/chiptune.h>

namespace Formats
{
  namespace Chiptune
  {
    namespace DigitalStudio
    {
      class Builder
      {
      public:
        virtual ~Builder() {}

        //common properties
        virtual void SetTitle(const String& title) = 0;
        virtual void SetProgram(const String& program) = 0;
        virtual void SetInitialTempo(uint_t tempo) = 0;
        //samples
        virtual void SetSample(uint_t index, std::size_t loop, Binary::Data::Ptr sample, bool is4Bit) = 0;
        //patterns
        virtual void SetPositions(const std::vector<uint_t>& positions, uint_t loop) = 0;
        //building pattern -> line -> channel
        //! @invariant Patterns are built sequentally
        virtual void StartPattern(uint_t index) = 0;
        //! @invariant Lines are built sequentally
        virtual void StartLine(uint_t index) = 0;
        virtual void SetTempo(uint_t tempo) = 0;
        //! @invariant Channels are built sequentally
        virtual void StartChannel(uint_t index) = 0;
        virtual void SetRest() = 0;
        virtual void SetNote(uint_t note) = 0;
        virtual void SetSample(uint_t sample) = 0;
      };

      Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
      Builder& GetStubBuilder();
    }
  }
}

#endif //FORMATS_CHIPTUNE_DIGITALSTUDIO_H_DEFINED
