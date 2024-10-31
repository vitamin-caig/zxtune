/**
 *
 * @file
 *
 * @brief  Formats::Chiptune::MetaBuilder adapter implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/properties_meta.h"

#include "module/attributes.h"

#include <string_view.h>

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

  void MetaProperties::SetPicture(Binary::View picture)
  {
    if (picture.Size() > PictureSize)
    {
      PictureSize = picture.Size();
      Delegate.SetBinaryProperty(ATTR_PICTURE, picture);
    }
  }
}  // namespace Module
