/**
* 
* @file
*
* @brief  Nested location implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "location.h"
//boost includes
#include <boost/make_shared.hpp>

namespace
{
  using namespace ZXTune;

  class NestedLocation : public DataLocation
  {
  public:
    NestedLocation(DataLocation::Ptr parent, const String& subPlugin, Binary::Container::Ptr subData, const String& subPath)
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

    virtual Analysis::Path::Ptr GetPluginsChain() const
    {
      return Parent->GetPluginsChain()->Append(SubPlugin);
    }
  private:
    const DataLocation::Ptr Parent;
    const Binary::Container::Ptr SubData;
    const String SubPlugin;
    const String Subpath;
  };
}

namespace ZXTune
{
  DataLocation::Ptr CreateNestedLocation(DataLocation::Ptr parent, Binary::Container::Ptr subData, const String& subPlugin, const String& subPath)
  {
    assert(subData);
    return boost::make_shared<NestedLocation>(parent, subPlugin, subData, subPath);
  }
}
