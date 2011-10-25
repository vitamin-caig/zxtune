/*
Abstract:
  Nested location implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "location.h"
//boost includes
#include <boost/make_shared.hpp>

namespace
{
  using namespace ZXTune;

  class NestedDataLocation : public DataLocation
  {
  public:
    NestedDataLocation(DataLocation::Ptr parent, Binary::Container::Ptr subData)
      : Parent(parent)
      , SubData(subData)
    {
    }

    virtual Binary::Container::Ptr GetData() const
    {
      return SubData;
    }

    virtual Analysis::Path::Ptr GetPath() const
    {
      return Parent->GetPath();
    }

    virtual PluginsChain::Ptr GetPlugins() const
    {
      return Parent->GetPlugins();
    }
  private:
    const DataLocation::Ptr Parent;
    const Binary::Container::Ptr SubData;
  };

  class NestedLocation : public DataLocation
  {
  public:
    NestedLocation(DataLocation::Ptr parent, Plugin::Ptr subPlugin, Binary::Container::Ptr subData, const String& subPath)
      : Parent(parent)
      , SubData(subData)
      , SubPlugin(subPlugin)
      , Subpath(subPath)
    {
    }

    virtual Binary::Container::Ptr GetData() const
    {
      return SubData;
    }

    virtual Analysis::Path::Ptr GetPath() const
    {
      return Parent->GetPath()->Append(Subpath);
    }

    virtual PluginsChain::Ptr GetPlugins() const
    {
      return PluginsChain::CreateMerged(Parent->GetPlugins(), SubPlugin);
    }
  private:
    const DataLocation::Ptr Parent;
    const Binary::Container::Ptr SubData;
    const Plugin::Ptr SubPlugin;
    const String Subpath;
  };
}

namespace ZXTune
{
  DataLocation::Ptr CreateNestedLocation(DataLocation::Ptr parent, Binary::Container::Ptr subData)
  {
    assert(subData);
    return boost::make_shared<NestedDataLocation>(parent, subData);
  }

  DataLocation::Ptr CreateNestedLocation(DataLocation::Ptr parent, Binary::Container::Ptr subData, Plugin::Ptr subPlugin, const String& subPath)
  {
    assert(subData);
    return boost::make_shared<NestedLocation>(parent, subPlugin, subData, subPath);
  }
}
