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

namespace
{
  using namespace Formats;

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

    virtual void SetGeometry(const CHS& /*geometry*/)
    {
    }

    virtual void SetSector(const CHS& location, const Dump& data)
    {
      if (Sectors.insert(SectorsMap::value_type(location, data)).second)
      {
        TotalSize += data.size();
      }
    }

    virtual Binary::Container::Ptr GetResult() const
    {
      std::auto_ptr<Dump> result(new Dump(TotalSize));
      Dump::iterator dst = result->begin();
      for (SectorsMap::const_iterator it = Sectors.begin(), lim = Sectors.end(); it != lim; ++it)
      {
        dst = std::copy(it->second.begin(), it->second.end(), dst);
      }
      return Binary::CreateContainer(result);
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
