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
    NestedLocation(DataLocation::Ptr parent, PluginId subPlugin, Binary::Container::Ptr subData, StringView subPath)
      : Parent(std::move(parent))
      , SubData(std::move(subData))
      , SubPlugin(subPlugin)
      , Subpath(subPath)
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
                                         StringView subPath)
  {
    return MakePtr<NestedLocation>(std::move(parent), subPlugin, std::move(subData), subPath);
  }
}  // namespace ZXTune
