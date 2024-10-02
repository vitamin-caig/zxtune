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
#include <string_view.h>
#include <types.h>
// library includes
#include <binary/view.h>
#include <strings/array.h>

namespace Formats::Chiptune
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
    virtual void SetPicture(Binary::View /*content*/){};
  };

  MetaBuilder& GetStubMetaBuilder();
}  // namespace Formats::Chiptune
