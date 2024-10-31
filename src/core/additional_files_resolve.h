/**
 *
 * @file
 *
 * @brief  Additional files resolving basic algorithm interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "binary/container.h"

#include "string_view.h"

namespace Module
{
  class AdditionalFiles;

  class AdditionalFilesSource
  {
  public:
    virtual ~AdditionalFilesSource() = default;

    //! @throws Error
    virtual Binary::Container::Ptr Get(StringView name) const = 0;
  };

  //! @throws Error
  void ResolveAdditionalFiles(const AdditionalFilesSource& source, const AdditionalFiles& files);
}  // namespace Module
