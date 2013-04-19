/*
Abstract:
  Patterns builder interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef FORMATS_CHIPTUNE_BUILDER_PATTERN_H_DEFINED
#define FORMATS_CHIPTUNE_BUILDER_PATTERN_H_DEFINED

//common includes
#include <types.h>

namespace Formats
{
  namespace Chiptune
  {
    class PatternBuilder
    {
    public:
      virtual ~PatternBuilder() {}

      virtual void Finish(uint_t size) = 0;
      //! @invariant Lines are built sequentally
      virtual void StartLine(uint_t index) = 0;
      virtual void SetTempo(uint_t tempo) = 0;
    };

    PatternBuilder& GetStubPatternBuilder();
  }
}

#endif //FORMATS_CHIPTUNE_BUILDER_PATTERN_H_DEFINED
