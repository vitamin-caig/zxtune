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

// common includes
#include <tools/progress_callback.h>

class QString;
namespace Playlist::IO
{
  class Container;

  enum ExportFlagValues
  {
    SAVE_ATTRIBUTES = 1,
    RELATIVE_PATHS = 2,
    SAVE_CONTENT = 4
  };

  using ExportFlags = uint_t;

  void SaveXSPF(const Container& container, const QString& filename, Log::ProgressCallback& cb, ExportFlags flags);
}  // namespace Playlist::IO
