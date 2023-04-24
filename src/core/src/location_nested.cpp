/**
 *
 * @file
 *
 * @brief  Nested location implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "core/src/location.h"
// common includes
#include <make_ptr.h>
// std includes
#include <utility>

namespace ZXTune
{
  class NestedLocation : public DataLocation
  {
  public:
    NestedLocation(DataLocation::Ptr parent, PluginId subPlugin, Binary::Container::Ptr subData, String subPath)
      : Parent(std::move(parent))
      , SubData(std::move(subData))
      , SubPlugin(std::move(subPlugin))
      , Subpath(std::move(subPath))
    {}

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
    const PluginId SubPlugin;
    const String Subpath;
  };
}  // namespace ZXTune

namespace ZXTune
{
  DataLocation::Ptr CreateNestedLocation(DataLocation::Ptr parent, Binary::Container::Ptr subData, PluginId subPlugin,
                                         const String& subPath)
  {
    assert(subData);
    return MakePtr<NestedLocation>(parent, subPlugin, subData, subPath);
  }
}  // namespace ZXTune
