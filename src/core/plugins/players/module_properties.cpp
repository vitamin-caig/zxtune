/**
* 
* @file
*
* @brief  Module properties builder implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "module_properties.h"
#include "core/plugins/enumerator.h"
#include "core/plugins/utils.h"
//library includes
#include <core/core_parameters.h>
#include <core/module_attrs.h>
//boost includes
#include <boost/make_shared.hpp>

namespace Module
{
  PropertiesBuilder::PropertiesBuilder()
      : Container(Parameters::Container::Create())
  {
  }

  //Visitor
  void PropertiesBuilder::SetValue(const Parameters::NameType& name, Parameters::IntType val)
  {
    Container->SetValue(name, val);
  }

  void PropertiesBuilder::SetValue(const Parameters::NameType& name, const Parameters::StringType& val)
  {
    Container->SetValue(name, val);
  }

  void PropertiesBuilder::SetValue(const Parameters::NameType& name, const Parameters::DataType& val)
  {
    Container->SetValue(name, val);
  }

  //MetaBuilder
  void PropertiesBuilder::SetProgram(const String& program)
  {
    const String optimizedProgram = OptimizeString(program);
    if (!optimizedProgram.empty())
    {
      Container->SetValue(ATTR_PROGRAM, optimizedProgram);
    }
  }

  void PropertiesBuilder::SetTitle(const String& title)
  {
    const String optimizedTitle = OptimizeString(title);
    if (!optimizedTitle.empty())
    {
      Container->SetValue(ATTR_TITLE, optimizedTitle);
    }
  }

  void PropertiesBuilder::SetAuthor(const String& author)
  {
    const String optimizedAuthor = OptimizeString(author);
    if (!optimizedAuthor.empty())
    {
      Container->SetValue(ATTR_AUTHOR, optimizedAuthor);
    }
  }

  //PropertiesBuilder
  void PropertiesBuilder::SetType(const String& type)
  {
    Container->SetValue(ATTR_TYPE, type);
  }

  void PropertiesBuilder::SetLocation(const ZXTune::DataLocation& location)
  {
    const String& container = location.GetPluginsChain()->AsString();
    if (!container.empty())
    {
      Container->SetValue(ATTR_CONTAINER, container);
    }
  }

  void PropertiesBuilder::SetSource(const Formats::Chiptune::Container& source)
  {
    Container->SetValue(ATTR_SIZE, source.Size());
    Container->SetValue(ATTR_CRC, source.Checksum());
    Container->SetValue(ATTR_FIXEDCRC, source.FixedChecksum());
  }

  void PropertiesBuilder::SetComment(const String& comment)
  {
    const String optimizedComment = OptimizeString(comment);
    if (!optimizedComment.empty())
    {
      Container->SetValue(ATTR_COMMENT, optimizedComment);
    }
  }

  void PropertiesBuilder::SetFreqtable(const String& table)
  {
    assert(!table.empty());
    Container->SetValue(Parameters::ZXTune::Core::AYM::TABLE, table);
  }

  void PropertiesBuilder::SetSamplesFreq(uint_t freq)
  {
    Container->SetValue(Parameters::ZXTune::Core::DAC::SAMPLES_FREQUENCY, freq);
  }

  void PropertiesBuilder::SetVersion(uint_t major, uint_t minor)
  {
    assert(minor < 10);
    const uint_t version = 10 * major + minor;
    Container->SetValue(ATTR_VERSION, version);
  }

  void PropertiesBuilder::SetVersion(const String& ver)
  {
    const String optimizedVersion = OptimizeString(ver);
    if (!optimizedVersion.empty())
    {
      Container->SetValue(ATTR_VERSION, optimizedVersion);
    }
  }
}
