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

// local includes
#include "container.h"
#include "playlist/supp/data_provider.h"
// common includes
#include <progress_callback.h>

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
