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

// local includes
#include "container.h"
#include "playlist/supp/data_provider.h"
// library includes
#include <parameters/accessor.h>

namespace Playlist::IO
{
  struct ContainerItem
  {
    String Path;
    Parameters::Accessor::Ptr AdjustedParameters;
  };

  struct ContainerItems : std::vector<ContainerItem>
  {
    typedef std::shared_ptr<const ContainerItems> Ptr;
    typedef std::shared_ptr<ContainerItems> RWPtr;
  };

  Container::Ptr CreateContainer(Item::DataProvider::Ptr provider, Parameters::Accessor::Ptr properties,
                                 ContainerItems::Ptr items);
}  // namespace Playlist::IO
