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
    ModulePropertiesImpl(const String& pluginId, DataLocation::Ptr location)
      : Container(Parameters::Container::Create())
      , Location(location)
    {
      Container->SetStringValue(ATTR_TYPE, pluginId);
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

    /*
    virtual void SetPath(const String& path)
    {
      Container->SetStringValue(ATTR_SUBPATH, path);
    }

    virtual void SetPlugins(PluginsChain::ConstPtr plugins)
    {
      Plugins = plugins;
    }

    virtual void SetSource(IO::DataContainer::Ptr src, const ModuleRegion& fixedRegion)
    {
      Source = src;
      FixedRegion = fixedRegion;
    }
    */
    virtual void SetSource(const ModuleRegion& usedRegion, const ModuleRegion& fixedRegion)
    {
      UsedRegion = usedRegion;
      FixedRegion = fixedRegion;
    }

    virtual bool FindIntValue(const Parameters::NameType& name, Parameters::IntType& val) const
    {
      CheckWarningsProperties(name);
      CheckLocationProperties(name);
      return Container->FindIntValue(name, val);
    }

    virtual bool FindStringValue(const Parameters::NameType& name, Parameters::StringType& val) const
    {
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
          name == ATTR_SIZE ||
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
      Container->SetIntValue(ATTR_SIZE, UsedRegion.Size);
      Container->SetIntValue(ATTR_CRC, UsedRegion.Checksum(*allData)); 
      Container->SetIntValue(ATTR_FIXEDCRC, FixedRegion.Checksum(*allData));
      //plugins
      const PluginsChain::ConstPtr plugins = Location->GetPlugins();
      const String& container = plugins->AsString();
      if (!container.empty())
      {
        Container->SetStringValue(ATTR_CONTAINER, container);
      }
      //path
      const String& subpath = Location->GetPath();
      if (!subpath.empty())
      {
        Container->SetStringValue(ATTR_SUBPATH, subpath);
      }
      Location.reset();
    }
  private:
    const Parameters::Container::Ptr Container;
    mutable Log::MessagesCollector::Ptr Warnings;
    mutable DataLocation::Ptr Location;
    ModuleRegion UsedRegion;
    ModuleRegion FixedRegion;
   };
}

namespace ZXTune
{
  namespace Module
  {
    ModuleProperties::Ptr ModuleProperties::Create(PlayerPlugin::Ptr plugin, DataLocation::Ptr location)
    {
      return boost::make_shared<ModulePropertiesImpl>(plugin->Id(), location);
    }
  }
}
