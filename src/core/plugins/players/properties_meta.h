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

//local includes
#include "properties_helper.h"
//library includes
#include <formats/chiptune/builder_meta.h>

namespace Module
{
  class MetaProperties : public Formats::Chiptune::MetaBuilder
  {
  public:
    explicit MetaProperties(PropertiesHelper& delegate)
      : Delegate(delegate)
    {
    }
    
    virtual void SetProgram(const String& program);
    virtual void SetTitle(const String& title);
    virtual void SetAuthor(const String& author);
    virtual void SetStrings(const Strings::Array& strings);

  private:
    PropertiesHelper& Delegate;
  };
}
