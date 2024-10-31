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

#include "apps/zxtune-qt/playlist/io/container.h"
#include "apps/zxtune-qt/playlist/supp/data_provider.h"

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
    using Ptr = std::shared_ptr<const ContainerItems>;
    using RWPtr = std::shared_ptr<ContainerItems>;
  };

  Container::Ptr CreateContainer(Item::DataProvider::Ptr provider, Parameters::Accessor::Ptr properties,
                                 ContainerItems::Ptr items);
}  // namespace Playlist::IO
