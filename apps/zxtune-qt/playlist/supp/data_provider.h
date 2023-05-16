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

// local includes
#include "data.h"
// common includes
#include <progress_callback.h>

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
    typedef std::shared_ptr<const DataProvider> Ptr;

    virtual ~DataProvider() = default;

    virtual void DetectModules(StringView path, DetectParameters& detectParams) const = 0;

    virtual void OpenModule(StringView path, DetectParameters& detectParams) const = 0;

    static Ptr Create(Parameters::Accessor::Ptr parameters);
  };
}  // namespace Playlist::Item
