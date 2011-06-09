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
    ModulePropertiesImpl(Plugin::Ptr plugin, DataLocation::Ptr location)
      : Container(Parameters::Container::Create())
      , Plug(plugin)
      , Location(location)
    {
    }

    virtual void SetTitle(const String& title)
    {
      if (!title.empty())
      {
        Container->SetStringValue(ATTR_TITLE, title);
      }
    }

    virtual void SetAuthor(const String& author)
    {
      if (!author.empty())
      {
        Container->SetStringValue(ATTR_AUTHOR, author);
      }
    }

    virtual void SetProgram(const String& program)
    {
      if (!program.empty())
      {
        Container->SetStringValue(ATTR_PROGRAM, program);
      }
    }

    virtual void SetWarnings(Log::MessagesCollector::Ptr warns)
    {
      Warnings = warns;
    }

    virtual void SetFreqtable(const String& table)
    {
      assert(!table.empty());
      Container->SetStringValue(Parameters::ZXTune::Core::AYM::TABLE, table);
    }

    virtual void SetVersion(uint_t major, uint_t minor)
    {
      assert(minor < 10);
      const uint_t version = 10 * major + minor;
      Container->SetIntValue(ATTR_VERSION, version);
    }

    virtual void SetSource(std::size_t usedSize, const ModuleRegion& fixedRegion)
    {
      UsedRegion = ModuleRegion(0, usedSize);
      FixedRegion = fixedRegion;
    }

    virtual Plugin::Ptr GetPlugin() const
    {
      return Plug;
    }

    virtual void GetData(Dump& dump) const
    {
      const IO::DataContainer::Ptr data = Location.get()
        ? UsedRegion.Extract(*Location->GetData())
        : Data;
      const uint8_t* const rawData = static_cast<const uint8_t*>(data->Data());
      dump.assign(rawData, rawData + data->Size());
    }

    // accessor virtuals
    virtual bool FindIntValue(const Parameters::NameType& name, Parameters::IntType& val) const
    {
      if (name == ATTR_SIZE)
      {
        val = UsedRegion.Size;
        return true;
      }
      CheckWarningsProperties(name);
      CheckLocationProperties(name);
      return Container->FindIntValue(name, val);
    }

    virtual bool FindStringValue(const Parameters::NameType& name, Parameters::StringType& val) const
    {
      if (name == ATTR_TYPE)
      {
        val = Plug->Id();
        return true;
      }
      CheckWarningsProperties(name);
      CheckLocationProperties(name);
      return Container->FindStringValue(name, val);
    }

    virtual bool FindDataValue(const Parameters::NameType& name, Parameters::DataType& val) const
    {
      return Container->FindDataValue(name, val);
    }

    virtual void Process(Parameters::Visitor& visitor) const
    {
      visitor.SetIntValue(ATTR_SIZE, UsedRegion.Size);
      visitor.SetStringValue(ATTR_TYPE, Plug->Id());
      FillWarningsProperties();
      FillLocationProperties();
      Container->Process(visitor);
    }

  private:
    void CheckWarningsProperties(const Parameters::NameType& name) const
    {
      if (Warnings.get() && 
          (name == ATTR_WARNINGS_COUNT ||
           name == ATTR_WARNINGS))
      {
        FillWarningsProperties();
      }
    }

    void FillWarningsProperties() const
    {
      if (!Warnings.get())
      {
        return;
      }
      if (const uint_t msgs = Warnings->CountMessages())
      {
        Container->SetIntValue(ATTR_WARNINGS_COUNT, msgs);
        Container->SetStringValue(ATTR_WARNINGS, Warnings->GetMessages('\n'));
      }
      Warnings.reset();
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
      const IO::DataContainer::Ptr allData = Location->GetData();
      Data = UsedRegion.Extract(*allData);
      Container->SetIntValue(ATTR_CRC, UsedRegion.Checksum(*allData)); 
      Container->SetIntValue(ATTR_FIXEDCRC, FixedRegion.Checksum(*allData));
      //plugins
      const PluginsChain::Ptr plugins = Location->GetPlugins();
      const String& container = plugins->AsString();
      if (!container.empty())
      {
        Container->SetStringValue(ATTR_CONTAINER, container);
      }
      //path
      const String& subpath = Location->GetPath()->AsString();
      if (!subpath.empty())
      {
        Container->SetStringValue(ATTR_SUBPATH, subpath);
      }
      Location.reset();
    }
  private:
    const Parameters::Container::Ptr Container;
    const Plugin::Ptr Plug;
    mutable Log::MessagesCollector::Ptr Warnings;
    mutable DataLocation::Ptr Location;
    ModuleRegion UsedRegion;
    ModuleRegion FixedRegion;
    mutable IO::DataContainer::Ptr Data;
   };
}

namespace ZXTune
{
  namespace Module
  {
    ModuleProperties::RWPtr ModuleProperties::Create(PlayerPlugin::Ptr plugin, DataLocation::Ptr location)
    {
      return boost::make_shared<ModulePropertiesImpl>(plugin, location);
    }
  }
}
