/**
 *
 * @file
 *
 * @brief  Formats::Chiptune::MetaBuilder adapter
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "formats/chiptune/builder_meta.h"  // IWYU pragma: export

#include "binary/view.h"
#include "strings/array.h"

#include "string_view.h"

#include <cstddef>

namespace Module
{
  class PropertiesHelper;

  class MetaProperties : public Formats::Chiptune::MetaBuilder
  {
  public:
    explicit MetaProperties(PropertiesHelper& delegate)
      : Delegate(delegate)
    {}

    void SetProgram(StringView program) override;
    void SetTitle(StringView title) override;
    void SetAuthor(StringView author) override;
    void SetStrings(const Strings::Array& strings) override;
    void SetComment(StringView comment) override;
    void SetPicture(Binary::View picture) override;

  private:
    PropertiesHelper& Delegate;
    std::size_t PictureSize = 0;
  };
}  // namespace Module
