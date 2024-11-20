/**
 *
 * @file
 *
 * @brief Playlist import implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune-qt/playlist/io/import.h"

#include "apps/zxtune-qt/playlist/io/container_impl.h"
#include "apps/zxtune-qt/ui/utils.h"

#include "make_ptr.h"

#include <QtCore/QStringList>

namespace
{
  Playlist::IO::ContainerItem CreateContainerItem(const QString& str)
  {
    Playlist::IO::ContainerItem res;
    res.Path = FromQString(str);
    res.AdjustedParameters = Parameters::Container::Create();
    return res;
  }
}  // namespace

namespace Playlist::IO
{
  Container::Ptr Open(Item::DataProvider::Ptr provider, const QString& filename, Log::ProgressCallback& cb)
  {
    if (auto ayl = OpenAYL(provider, filename, cb))
    {
      return ayl;
    }
    else if (auto xspf = OpenXSPF(std::move(provider), filename, cb))
    {
      return xspf;
    }
    return {};
  }

  Container::Ptr OpenPlainList(Item::DataProvider::Ptr provider, const QStringList& uris)
  {
    auto items = MakeRWPtr<ContainerItems>();
    std::transform(uris.begin(), uris.end(), std::back_inserter(*items), &CreateContainerItem);
    auto props = Parameters::Container::Create();
    props->SetValue(ATTRIBUTE_ITEMS, items->size());
    return CreateContainer(std::move(provider), std::move(props), std::move(items));
  }
}  // namespace Playlist::IO
