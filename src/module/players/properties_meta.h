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

#include "formats/chiptune/builder_meta.h"
#include "module/players/properties_helper.h"

#include "string_view.h"

namespace Module
{
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
