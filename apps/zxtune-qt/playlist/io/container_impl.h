/**
* 
* @file
*
* @brief Playlist container internal interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "container.h"
#include "playlist/supp/data_provider.h"
//library includes
#include <parameters/accessor.h>

namespace Playlist
{
  namespace IO
  {
    struct ContainerItem
    {
      String Path;
      Parameters::Accessor::Ptr AdjustedParameters;
    };

    typedef std::vector<ContainerItem> ContainerItems;
    typedef boost::shared_ptr<const ContainerItems> ContainerItemsPtr;

    Container::Ptr CreateContainer(Item::DataProvider::Ptr provider,
      Parameters::Accessor::Ptr properties,
      ContainerItemsPtr items);
  }
}
