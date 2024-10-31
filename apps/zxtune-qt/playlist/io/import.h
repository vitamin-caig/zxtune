/**
 *
 * @file
 *
 * @brief Playlist import interfaces
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "apps/zxtune-qt/playlist/io/container.h"
#include "apps/zxtune-qt/playlist/supp/data_provider.h"

#include "tools/progress_callback.h"

class QString;
class QStringList;

namespace Playlist::IO
{
  // common
  Container::Ptr Open(Item::DataProvider::Ptr provider, const QString& filename, Log::ProgressCallback& cb);
  // specific
  Container::Ptr OpenAYL(Item::DataProvider::Ptr provider, const QString& filename, Log::ProgressCallback& cb);
  Container::Ptr OpenXSPF(Item::DataProvider::Ptr provider, const QString& filename, Log::ProgressCallback& cb);

  Container::Ptr OpenPlainList(Item::DataProvider::Ptr provider, const QStringList& uris);
}  // namespace Playlist::IO
