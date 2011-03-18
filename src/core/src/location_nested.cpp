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
//library includes
#include <io/fs_tools.h>
//boost includes
#include <boost/make_shared.hpp>

namespace
{
  using namespace ZXTune;

  class NestedDataLocation : public DataLocation
  {
  public:
    NestedDataLocation(DataLocation::Ptr parent, IO::DataContainer::Ptr subData)
      : Parent(parent)
      , SubData(subData)
    {
    }

    virtual IO::DataContainer::Ptr GetData() const
    {
      return SubData;
    }

    virtual String GetPath() const
    {
      return Parent->GetPath();
    }

    virtual PluginsChain::ConstPtr GetPlugins() const
    {
      return Parent->GetPlugins();
    }
  private:
    const DataLocation::Ptr Parent;
    const IO::DataContainer::Ptr SubData;
  };

  class NestedLocation : public DataLocation
  {
  public:
    NestedLocation(DataLocation::Ptr parent, Plugin::Ptr subPlugin, IO::DataContainer::Ptr subData, const String& subPath)
      : Parent(parent)
      , SubData(subData)
      , SubPlugin(subPlugin)
      , SubPath(subPath)
    {
    }

    virtual IO::DataContainer::Ptr GetData() const
    {
      return SubData;
    }

    virtual String GetPath() const
    {
      return IO::AppendPath(Parent->GetPath(), SubPath);
    }

    virtual PluginsChain::ConstPtr GetPlugins() const
    {
      return PluginsChain::CreateMerged(Parent->GetPlugins(), SubPlugin);
    }
  private:
    const DataLocation::Ptr Parent;
    const IO::DataContainer::Ptr SubData;
    const Plugin::Ptr SubPlugin;
    const String SubPath;
  };
}

namespace ZXTune
{
  DataLocation::Ptr CreateNestedLocation(DataLocation::Ptr parent, IO::DataContainer::Ptr subData)
  {
    return boost::make_shared<NestedDataLocation>(parent, subData);
  }

  DataLocation::Ptr CreateNestedLocation(DataLocation::Ptr parent, Plugin::Ptr subPlugin, IO::DataContainer::Ptr subData, const String& subPath)
  {
    return boost::make_shared<NestedLocation>(parent, subPlugin, subData, subPath);
  }
}
