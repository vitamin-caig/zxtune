/*
Abstract:
  Module properties container implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "module_properties.h"
#include "core/plugins/enumerator.h"
#include "core/plugins/utils.h"
//library includes
#include <core/core_parameters.h>
#include <core/module_attrs.h>
//boost includes
#include <boost/make_shared.hpp>

namespace ZXTune
{
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

  void PropertiesBuilder::SetLocation(DataLocation::Ptr location)
  {
    const String& container = location->GetPluginsChain()->AsString();
    if (!container.empty())
    {
      Container->SetValue(ATTR_CONTAINER, container);
    }
  }

  void PropertiesBuilder::SetSource(Binary::Data::Ptr data, std::size_t usedSize, const ModuleRegion& fixedRegion)
  {
    Container->SetValue(ATTR_SIZE, usedSize);
    const ModuleRegion usedRegion = ModuleRegion(0, usedSize);
    Container->SetValue(ATTR_CRC, usedRegion.Checksum(*data));
    Container->SetValue(ATTR_FIXEDCRC, fixedRegion.Checksum(*data));
    const uint8_t* const start = static_cast<const uint8_t*>(data->Start());
    Container->SetValue(ATTR_CONTENT, Parameters::DataType(start, start + usedSize));
  }

  void PropertiesBuilder::SetSource(Formats::Chiptune::Container::Ptr source)
  {
    Container->SetValue(ATTR_SIZE, source->Size());
    Container->SetValue(ATTR_CRC, source->Checksum());
    Container->SetValue(ATTR_FIXEDCRC, source->FixedChecksum());
    const uint8_t* const start = static_cast<const uint8_t*>(source->Start());
    Container->SetValue(ATTR_CONTENT, Parameters::DataType(start, start + source->Size()));
  }

  void PropertiesBuilder::SetComment(const String& comment)
  {
    if (!comment.empty())
    {
      Container->SetValue(ATTR_COMMENT, comment);
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
    if (!ver.empty())
    {
      Container->SetValue(ATTR_VERSION, ver);
    }
  }
}
}
