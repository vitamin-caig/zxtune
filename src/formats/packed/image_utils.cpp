/**
*
* @file
*
* @brief  Image utilities implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "image_utils.h"
//common includes
#include <make_ptr.h>
//library includes
#include <binary/container_factories.h>
//std includes
#include <map>

namespace Formats
{
  bool CompareCHS(const CHS& lh, const CHS& rh)
  {
    return lh.Cylinder == rh.Cylinder
      ? (lh.Head == rh.Head
         ? lh.Sector < rh.Sector
         : lh.Head < rh.Head)
      : lh.Cylinder < rh.Cylinder;
  }
}

namespace Formats
{
  class SparsedImageBuilder : public ImageBuilder
  {
  public:
    SparsedImageBuilder()
      : Sectors(&CompareCHS)
      , TotalSize()
    {
    }

    void SetGeometry(const CHS& /*geometry*/) override
    {
    }

    void SetSector(const CHS& location, Dump data) override
    {
      const auto res = Sectors.insert(SectorsMap::value_type(location, std::move(data)));
      if (res.second)
      {
        TotalSize += res.first->second.size();
      }
    }

    Binary::Container::Ptr GetResult() const override
    {
      std::unique_ptr<Dump> result(new Dump(TotalSize));
      auto dst = result->begin();
      for (const auto& sec : Sectors)
      {
        dst = std::copy(sec.second.begin(), sec.second.end(), dst);
      }
      return Binary::CreateContainer(std::move(result));
    }
  private:
    typedef std::map<CHS, Dump, bool(*)(const CHS&, const CHS&)> SectorsMap;
    SectorsMap Sectors;
    std::size_t TotalSize;
  };

  ImageBuilder::Ptr CreateSparsedImageBuilder()
  {
    return MakePtr<SparsedImageBuilder>();
  }
}
