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

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  class ModulePropertiesImpl : public ModuleProperties
  {
  public:
    ModulePropertiesImpl(const String& type, DataLocation::Ptr location)
      : Container(Parameters::Container::Create())
      , Type(type)
      , Location(location)
    {
    }

    virtual void SetTitle(const String& title)
    {
      const String optimizedTitle = OptimizeString(title);
      if (!optimizedTitle.empty())
      {
        Container->SetValue(ATTR_TITLE, optimizedTitle);
      }
    }

    virtual void SetAuthor(const String& author)
    {
      const String optimizedAuthor = OptimizeString(author);
      if (!optimizedAuthor.empty())
      {
        Container->SetValue(ATTR_AUTHOR, optimizedAuthor);
      }
    }

    virtual void SetComment(const String& comment)
    {
      if (!comment.empty())
      {
        Container->SetValue(ATTR_COMMENT, comment);
      }
    }

    virtual void SetProgram(const String& program)
    {
      const String optimizedProgram = OptimizeString(program);
      if (!optimizedProgram.empty())
      {
        Container->SetValue(ATTR_PROGRAM, optimizedProgram);
      }
    }

    virtual void SetFreqtable(const String& table)
    {
      assert(!table.empty());
      Container->SetValue(Parameters::ZXTune::Core::AYM::TABLE, table);
    }

    virtual void SetSamplesFreq(uint_t freq)
    {
      Container->SetValue(Parameters::ZXTune::Core::DAC::SAMPLES_FREQUENCY, freq);
    }

    virtual void SetVersion(uint_t major, uint_t minor)
    {
      assert(minor < 10);
      const uint_t version = 10 * major + minor;
      Container->SetValue(ATTR_VERSION, version);
    }

    virtual void SetSource(std::size_t usedSize, const ModuleRegion& fixedRegion)
    {
      UsedRegion = ModuleRegion(0, usedSize);
      FixedRegion = fixedRegion;
    }

    virtual void SetSource(Formats::Chiptune::Container::Ptr source)
    {
      UsedRegion = ModuleRegion(0, source->Size());
      Source = source;
    }

    virtual Parameters::Modifier::Ptr GetInternalContainer() const
    {
      return Container;
    }

    // accessor virtuals
    virtual bool FindValue(const Parameters::NameType& name, Parameters::IntType& val) const
    {
      if (name == ATTR_SIZE)
      {
        val = UsedRegion.Size;
        return true;
      }
      CheckLocationProperties(name);
      return Container->FindValue(name, val);
    }

    virtual bool FindValue(const Parameters::NameType& name, Parameters::StringType& val) const
    {
      if (name == ATTR_TYPE)
      {
        val = Type;
        return true;
      }
      CheckLocationProperties(name);
      return Container->FindValue(name, val);
    }

    virtual bool FindValue(const Parameters::NameType& name, Parameters::DataType& val) const
    {
      if (name == ATTR_CONTENT)
      {
        const Binary::Data::Ptr data = GetData();
        const uint8_t* const start = static_cast<const uint8_t*>(data->Start());
        val = Parameters::DataType(start, start + data->Size());
        return true;
      }
      return Container->FindValue(name, val);
    }

    virtual void Process(Parameters::Visitor& visitor) const
    {
      visitor.SetValue(ATTR_SIZE, UsedRegion.Size);
      visitor.SetValue(ATTR_TYPE, Type);
      FillLocationProperties();
      Container->Process(visitor);
    }

  private:
    Binary::Data::Ptr GetData() const
    {
      return Location.get()
        ? UsedRegion.Extract(*Location->GetData())
        : Data;
    }

    void CheckLocationProperties(const Parameters::NameType& name) const
    {
      if (!Location.get())
      {
        return;
      }
      if (//data
          name == ATTR_CRC ||
          name == ATTR_FIXEDCRC ||
          //plugins
          name == ATTR_CONTAINER ||
          //path
          name == ATTR_SUBPATH
          )
      {
        FillLocationProperties();
      }
    }

    void FillLocationProperties() const
    {
      if (!Location.get())
      {
        return;
      }
      //data
      const Binary::Container::Ptr allData = Location->GetData();
      Data = UsedRegion.Extract(*allData);
      Container->SetValue(ATTR_CRC, UsedRegion.Checksum(*allData)); 
      if (Source)
      {
        Container->SetValue(ATTR_FIXEDCRC, Source->FixedChecksum());
        Source.reset();
      }
      else
      {
        Container->SetValue(ATTR_FIXEDCRC, FixedRegion.Checksum(*allData));
      }
      //plugins
      const String& container = Location->GetPluginsChain()->AsString();
      if (!container.empty())
      {
        Container->SetValue(ATTR_CONTAINER, container);
      }
      //path
      const String& subpath = Location->GetPath()->AsString();
      if (!subpath.empty())
      {
        Container->SetValue(ATTR_SUBPATH, subpath);
      }
      Location.reset();
    }
  private:
    const Parameters::Container::Ptr Container;
    const String Type;
    mutable DataLocation::Ptr Location;
    mutable Formats::Chiptune::Container::Ptr Source;
    ModuleRegion UsedRegion;
    ModuleRegion FixedRegion;
    mutable Binary::Container::Ptr Data;
   };
}

namespace ZXTune
{
  namespace Module
  {
    ModuleProperties::RWPtr ModuleProperties::Create(const String& type, DataLocation::Ptr location)
    {
      return boost::make_shared<ModulePropertiesImpl>(type, location);
    }
  }
}
