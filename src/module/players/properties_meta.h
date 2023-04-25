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

// local includes
#include "module/players/properties_helper.h"
// library includes
#include <formats/chiptune/builder_meta.h>

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

  private:
    PropertiesHelper& Delegate;
  };
}  // namespace Module
