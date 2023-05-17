/**
 *
 * @file
 *
 * @brief Playlist export interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// local includes
#include "container.h"
// common includes
#include <progress_callback.h>

class QString;
namespace Playlist::IO
{
  enum ExportFlagValues
  {
    SAVE_ATTRIBUTES = 1,
    RELATIVE_PATHS = 2,
    SAVE_CONTENT = 4
  };

  using ExportFlags = uint_t;

  void SaveXSPF(Container::Ptr container, const QString& filename, Log::ProgressCallback& cb, ExportFlags flags);
}  // namespace Playlist::IO
