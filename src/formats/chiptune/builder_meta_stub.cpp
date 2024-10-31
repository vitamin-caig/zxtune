/**
 *
 * @file
 *
 * @brief  Metadata builder stub implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/builder_meta.h"

#include <string_view.h>

namespace Formats::Chiptune
{
  class StubMetaBuilder : public MetaBuilder
  {
  public:
    void SetProgram(StringView /*program*/) override {}
    void SetTitle(StringView /*title*/) override {}
    void SetAuthor(StringView /*author*/) override {}
    void SetStrings(const Strings::Array& /*strings*/) override {}
    void SetComment(StringView /*comment*/) override {}
  };

  MetaBuilder& GetStubMetaBuilder()
  {
    static StubMetaBuilder instance;
    return instance;
  }
}  // namespace Formats::Chiptune
