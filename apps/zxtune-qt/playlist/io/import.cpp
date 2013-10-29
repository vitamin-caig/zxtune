/**
* 
* @file
*
* @brief Playlist import implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "import.h"
#include "container_impl.h"
#include "ui/utils.h"
//boost includes
#include <boost/make_shared.hpp>
//qt includes
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
}

namespace Playlist
{
  namespace IO
  {
    Container::Ptr Open(Item::DataProvider::Ptr provider, const QString& filename)
    {
      if (Container::Ptr res = OpenAYL(provider, filename))
      {
        return res;
      }
      else if (Container::Ptr res = OpenXSPF(provider, filename))
      {
        return res;
      }
      return Container::Ptr();
    }

    Container::Ptr OpenPlainList(Item::DataProvider::Ptr provider, const QStringList& uris)
    {
      const boost::shared_ptr<ContainerItems> items = boost::make_shared<ContainerItems>();
      std::transform(uris.begin(), uris.end(), std::back_inserter(*items), &CreateContainerItem);
      const Parameters::Container::Ptr props = Parameters::Container::Create();
      props->SetValue(ATTRIBUTE_ITEMS, items->size());
      return CreateContainer(provider, props, items);
    }
  }
}
