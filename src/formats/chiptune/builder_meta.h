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

// common includes
#include <types.h>
// library includes
#include <strings/array.h>

namespace Formats
{
  namespace Chiptune
  {
    class MetaBuilder
    {
    public:
      virtual ~MetaBuilder() = default;

      virtual void SetProgram(StringView program) = 0;
      virtual void SetTitle(StringView title) = 0;
      virtual void SetAuthor(StringView author) = 0;
      virtual void SetStrings(const Strings::Array& strings) = 0;
      virtual void SetComment(StringView comment) = 0;
    };

    MetaBuilder& GetStubMetaBuilder();
  }  // namespace Chiptune
}  // namespace Formats
