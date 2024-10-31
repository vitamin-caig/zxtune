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

#include "apps/zxtune-qt/playlist/supp/model.h"

namespace Log
{
  class ProgressCallback;
}  // namespace Log

namespace Playlist::Item
{
  void ExecuteOperation(const class Storage& stor, const Model::IndexSet::Ptr& selectedItems, class Visitor& visitor,
                        Log::ProgressCallback& cb);
}  // namespace Playlist::Item
