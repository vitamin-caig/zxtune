/**
 *
 * @file
 *
 * @brief  Formats::Chiptune::MetaBuilder adapter implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "module/players/properties_meta.h"

namespace Module
{
  void MetaProperties::SetProgram(StringView program)
  {
    Delegate.SetProgram(program);
  }

  void MetaProperties::SetTitle(StringView title)
  {
    Delegate.SetTitle(title);
  }

  void MetaProperties::SetAuthor(StringView author)
  {
    Delegate.SetAuthor(author);
  }

  void MetaProperties::SetStrings(const Strings::Array& strings)
  {
    Delegate.SetStrings(strings);
  }

  void MetaProperties::SetComment(StringView comment)
  {
    Delegate.SetComment(comment);
  }
}  // namespace Module
