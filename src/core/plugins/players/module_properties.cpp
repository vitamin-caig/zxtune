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
    explicit ModulePropertiesImpl(const String& pluginId)
      : Container(Parameters::Container::Create())
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

    virtual bool FindIntValue(const Parameters::NameType& name, Parameters::IntType& val) const
    {
      CheckWarningsProperties(name);
      CheckSourceProperties(name);
      return Container->FindIntValue(name, val);
    }

    virtual bool FindStringValue(const Parameters::NameType& name, Parameters::StringType& val) const
    {
      CheckWarningsProperties(name);
      CheckPluginsProperties(name);
      return Container->FindStringValue(name, val);
    }

    virtual bool FindDataValue(const Parameters::NameType& name, Parameters::DataType& val) const
    {
      return Container->FindDataValue(name, val);
    }

    virtual void Process(Parameters::Visitor& visitor) const
    {
      FillWarningsProperties();
      FillPluginsProperties();
      FillSourceProperties();
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

    void CheckPluginsProperties(const Parameters::NameType& name) const
    {
      if (Plugins.get() &&
          name == ATTR_CONTAINER)
      {
        FillPluginsProperties();
      }
    }

    void FillPluginsProperties() const
    {
      if (!Plugins.get())
      {
        return;
      }
      const String& plugins = Plugins->AsString();
      if (!plugins.empty())
      {
        Container->SetStringValue(ATTR_CONTAINER, plugins);
      }
      Plugins.reset();
    }

    void CheckSourceProperties(const Parameters::NameType& name) const
    {
      if (Source.get() &&
          (name == ATTR_CRC ||
           name == ATTR_SIZE ||
           name == ATTR_FIXEDCRC))
      {
        FillSourceProperties();
      }
    }

    void FillSourceProperties() const
    {
      if (!Source.get())
      {
        return;
      }
      const ModuleRegion allRegion(0, Source->Size());
      Container->SetIntValue(ATTR_SIZE, allRegion.Size);
      Container->SetIntValue(ATTR_CRC, allRegion.Checksum(*Source)); 
      Container->SetIntValue(ATTR_FIXEDCRC, FixedRegion.Checksum(*Source)); 
      Source.reset();
    }
  private:
    const Parameters::Container::Ptr Container;
    mutable Log::MessagesCollector::Ptr Warnings;
    mutable PluginsChain::ConstPtr Plugins;
    mutable IO::DataContainer::Ptr Source;
    ModuleRegion FixedRegion;
  };
}

namespace ZXTune
{
  namespace Module
  {
    ModuleProperties::Ptr ModuleProperties::Create(const String& pluginId)
    {
      return boost::make_shared<ModulePropertiesImpl>(pluginId);
    }
  }
}
