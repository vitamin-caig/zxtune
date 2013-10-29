/**
* 
* @file
*
* @brief  Metadata builder interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

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
