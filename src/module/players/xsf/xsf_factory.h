/**
 *
 * @file
 *
 * @brief  Xsf-based files common code
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/players/factory.h"
#include "module/players/xsf/xsf_file.h"

#include "module/holder.h"
#include "parameters/container.h"
#include "strings/map.h"

#include <memory>

namespace Module::XSF
{
  using FilesMap = Strings::ValueMap<File>;

  class Factory
  {
  public:
    // May be used across multiple plugins
    using Ptr = std::shared_ptr<const Factory>;
    virtual ~Factory() = default;

    virtual Holder::Ptr CreateSinglefileModule(const File& file, Parameters::Container::Ptr properties) const = 0;
    virtual Holder::Ptr CreateMultifileModule(const File& file, const FilesMap& additionalFiles,
                                              Parameters::Container::Ptr properties) const = 0;
  };

  Module::Factory::Ptr CreateModuleFactory(XSF::Factory::Ptr delegate);
}  // namespace Module::XSF
