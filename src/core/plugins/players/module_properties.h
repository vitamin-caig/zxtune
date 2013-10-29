/**
* 
* @file
*
* @brief  Module properties builder interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "core/src/location.h"
//library includes
#include <formats/chiptune.h>
#include <formats/chiptune/builder_meta.h>
#include <parameters/container.h>

namespace Module
{
  class PropertiesBuilder : public Parameters::Visitor, public Formats::Chiptune::MetaBuilder
  {
  public:
    PropertiesBuilder();

    virtual void SetValue(const Parameters::NameType& name, Parameters::IntType val);
    virtual void SetValue(const Parameters::NameType& name, const Parameters::StringType& val);
    virtual void SetValue(const Parameters::NameType& name, const Parameters::DataType& val);

    virtual void SetProgram(const String& program);
    virtual void SetTitle(const String& title);
    virtual void SetAuthor(const String& author);

    void SetType(const String& type);
    void SetLocation(const ZXTune::DataLocation& location);
    void SetSource(const Formats::Chiptune::Container& source);
    void SetComment(const String& comment);
    void SetFreqtable(const String& table);
    void SetSamplesFreq(uint_t freq);
    void SetVersion(uint_t major, uint_t minor);
    void SetVersion(const String& version);

    Parameters::Accessor::Ptr GetResult() const
    {
      return Container;
    }
  private:
    const Parameters::Container::Ptr Container;
  };
}
