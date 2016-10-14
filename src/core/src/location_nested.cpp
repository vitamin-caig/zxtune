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
//common includes
#include <make_ptr.h>

namespace ZXTune
{
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

    Binary::Container::Ptr GetData() const override
    {
      return SubData;
    }

    Analysis::Path::Ptr GetPath() const override
    {
      return Parent->GetPath()->Append(Subpath);
    }

    Analysis::Path::Ptr GetPluginsChain() const override
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
    return MakePtr<NestedLocation>(parent, subPlugin, subData, subPath);
  }
}
