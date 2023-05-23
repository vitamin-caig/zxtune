/**
 *
 * @file
 *
 * @brief Playlist import implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "import.h"
#include "container_impl.h"
#include "ui/utils.h"
// common includes
#include <make_ptr.h>
// qt includes
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
    return Container::Ptr();
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
