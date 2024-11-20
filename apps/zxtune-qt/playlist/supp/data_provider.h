/**
 *
 * @file
 *
 * @brief Playlist data provider interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "apps/zxtune-qt/playlist/supp/data.h"

#include "tools/progress_callback.h"

#include "string_view.h"

namespace Playlist::Item
{
  class DetectParameters
  {
  public:
    virtual ~DetectParameters() = default;

    virtual Parameters::Container::Ptr CreateInitialAdjustedParameters() const = 0;
    virtual void ProcessItem(Data::Ptr item) = 0;
    virtual Log::ProgressCallback* GetProgress() const = 0;
  };

  class DataProvider
  {
  public:
    using Ptr = std::shared_ptr<const DataProvider>;

    virtual ~DataProvider() = default;

    virtual void DetectModules(StringView path, DetectParameters& detectParams) const = 0;

    virtual void OpenModule(StringView path, DetectParameters& detectParams) const = 0;

    static Ptr Create(Parameters::Accessor::Ptr parameters);
  };
}  // namespace Playlist::Item
