/**
 *
 * @file
 *
 * @brief Playlist operations helpers interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// local includes
#include "model.h"

namespace Playlist::Item
{
  void ExecuteOperation(const class Storage& stor, Model::IndexSet::Ptr selectedItems, class Visitor& visitor,
                        Log::ProgressCallback& cb);
}  // namespace Playlist::Item
