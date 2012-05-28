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
        Container->SetValue(ATTR_TITLE, title);
      }
    }

    virtual void SetAuthor(const String& author)
    {
      if (!author.empty())
      {
        Container->SetValue(ATTR_AUTHOR, author);
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
      if (!program.empty())
      {
        Container->SetValue(ATTR_PROGRAM, program);
      }
    }

    virtual void SetWarnings(Log::MessagesCollector::Ptr warns)
    {
      Warnings = warns;
    }

    virtual void SetFreqtable(const String& table)
    {
      assert(!table.empty());
      Container->SetValue(Parameters::ZXTune::Core::AYM::TABLE, table);
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

    virtual Plugin::Ptr GetPlugin() const
    {
      return Plug;
    }

    virtual void GetData(Dump& dump) const
    {
      const Binary::Container::Ptr data = Location.get()
        ? UsedRegion.Extract(*Location->GetData())
        : Data;
      const uint8_t* const rawData = static_cast<const uint8_t*>(data->Data());
      dump.assign(rawData, rawData + data->Size());
    }

    // accessor virtuals
    virtual bool FindValue(const Parameters::NameType& name, Parameters::IntType& val) const
    {
      if (name == ATTR_SIZE)
      {
        val = UsedRegion.Size;
        return true;
      }
      CheckWarningsProperties(name);
      CheckLocationProperties(name);
      return Container->FindValue(name, val);
    }

    virtual bool FindValue(const Parameters::NameType& name, Parameters::StringType& val) const
    {
      if (name == ATTR_TYPE)
      {
        val = Plug->Id();
        return true;
      }
      CheckWarningsProperties(name);
      CheckLocationProperties(name);
      return Container->FindValue(name, val);
    }

    virtual bool FindValue(const Parameters::NameType& name, Parameters::DataType& val) const
    {
      return Container->FindValue(name, val);
    }

    virtual void Process(Parameters::Visitor& visitor) const
    {
      visitor.SetValue(ATTR_SIZE, UsedRegion.Size);
      visitor.SetValue(ATTR_TYPE, Plug->Id());
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
        Container->SetValue(ATTR_WARNINGS_COUNT, msgs);
        Container->SetValue(ATTR_WARNINGS, Warnings->GetMessages('\n'));
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
      const PluginsChain::Ptr plugins = Location->GetPlugins();
      const String& container = plugins->AsString();
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
    const Plugin::Ptr Plug;
    mutable Log::MessagesCollector::Ptr Warnings;
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
    ModuleProperties::RWPtr ModuleProperties::Create(Plugin::Ptr plugin, DataLocation::Ptr location)
    {
      return boost::make_shared<ModulePropertiesImpl>(plugin, location);
    }
  }
}
