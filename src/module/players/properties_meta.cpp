/**
* 
* @file
*
* @brief  Formats::Chiptune::MetaBuilder adapter implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "properties_meta.h"

namespace Module
{
  void MetaProperties::SetProgram(const String& program)
  {
    Delegate.SetProgram(program);
  }

  void MetaProperties::SetTitle(const String& title)
  {
    Delegate.SetTitle(title);
  }

  void MetaProperties::SetAuthor(const String& author)
  {
    Delegate.SetAuthor(author);
  }
  
  void MetaProperties::SetStrings(const Strings::Array& strings)
  {
    Delegate.SetStrings(strings);
  }
}
