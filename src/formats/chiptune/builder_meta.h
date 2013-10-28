/*
Abstract:
  Meta builder interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef FORMATS_CHIPTUNE_BUILDER_META_H_DEFINED
#define FORMATS_CHIPTUNE_BUILDER_META_H_DEFINED

//common includes
#include <types.h>

namespace Formats
{
  namespace Chiptune
  {
    class MetaBuilder
    {
    public:
      virtual ~MetaBuilder() {}

      virtual void SetProgram(const String& program) = 0;
      virtual void SetTitle(const String& title) = 0;
      virtual void SetAuthor(const String& author) = 0;
    };

    MetaBuilder& GetStubMetaBuilder();
  }
}

#endif //FORMATS_CHIPTUNE_BUILDER_META_H_DEFINED
