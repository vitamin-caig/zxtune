/*
Abstract:
  TFD modules format description

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef FORMATS_CHIPTUNE_TFD_H_DEFINED
#define FORMATS_CHIPTUNE_TFD_H_DEFINED

//common includes
#include <types.h>
//library includes
#include <formats/chiptune.h>

namespace Formats
{
  namespace Chiptune
  {
    namespace TFD
    {
      class Builder
      {
      public:
        typedef boost::shared_ptr<Builder> Ptr;
        virtual ~Builder() {}

        virtual void SetTitle(const String& title) = 0;
        virtual void SetAuthor(const String& author) = 0;
        virtual void SetComment(const String& comment) = 0;

        virtual void BeginFrames(uint_t count) = 0;
        virtual void SelectChip(uint_t idx) = 0;
        virtual void SetLoop() = 0;
        virtual void SetRegister(uint_t idx, uint_t val) = 0;
      };

      Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
      Builder& GetStubBuilder();
    }
  }
}

#endif //FORMATS_CHIPTUNE_TFD_H_DEFINED
