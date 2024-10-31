/**
 *
 * @file
 *
 * @brief  Image utilities implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/packed/image_utils.h"

#include "binary/data_builder.h"

#include "make_ptr.h"

#include <map>
#include <utility>

namespace Formats
{
  bool CompareCHS(const CHS& lh, const CHS& rh)
  {
    return lh.Cylinder == rh.Cylinder ? (lh.Head == rh.Head ? lh.Sector < rh.Sector : lh.Head < rh.Head)
                                      : lh.Cylinder < rh.Cylinder;
  }
}  // namespace Formats

namespace Formats
{
  class SparsedImageBuilder : public ImageBuilder
  {
  public:
    SparsedImageBuilder()
      : Sectors(&CompareCHS)
    {}

    void SetGeometry(const CHS& /*geometry*/) override {}

    void SetSector(const CHS& location, Binary::View data) override
    {
      const auto res = Sectors.insert(SectorsMap::value_type(location, data));
      if (res.second)
      {
        TotalSize += res.first->second.Size();
      }
    }

    Binary::Container::Ptr GetResult() const override
    {
      Binary::DataBuilder result(TotalSize);
      for (const auto& sec : Sectors)
      {
        result.Add(sec.second);
      }
      return result.CaptureResult();
    }

  private:
    using SectorsMap = std::map<CHS, Binary::View, bool (*)(const CHS&, const CHS&)>;
    SectorsMap Sectors;
    std::size_t TotalSize = 0;
  };

  ImageBuilder::Ptr CreateSparsedImageBuilder()
  {
    return MakePtr<SparsedImageBuilder>();
  }
}  // namespace Formats
