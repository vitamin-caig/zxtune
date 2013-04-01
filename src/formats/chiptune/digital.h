/*
Abstract:
  Base interfaces for simple digital trackers (only samples, without effects)

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef FORMATS_CHIPTUNE_DIGITAL_H_DEFINED
#define FORMATS_CHIPTUNE_DIGITAL_H_DEFINED

//local includes
#include "builder_meta.h"
//common includes
#include <types.h>
//library includes
#include <binary/container.h>

namespace Formats
{
  namespace Chiptune
  {
    namespace Digital
    {
      class Builder
      {
      public:
        virtual ~Builder() {}

        virtual MetaBuilder& GetMetaBuilder() = 0;
        //common properties
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

      Builder& GetStubBuilder();
    }
  }
}

#endif //FORMATS_CHIPTUNE_DIGITAL_H_DEFINED
