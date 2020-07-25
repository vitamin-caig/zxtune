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
#include "module/players/properties_helper.h"
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
    
    void SetProgram(const String& program) override;
    void SetTitle(const String& title) override;
    void SetAuthor(const String& author) override;
    void SetStrings(const Strings::Array& strings) override;

  private:
    PropertiesHelper& Delegate;
  };
}
