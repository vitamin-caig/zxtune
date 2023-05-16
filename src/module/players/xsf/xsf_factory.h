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

// local includes
#include "module/players/xsf/xsf_file.h"
// library includes
#include <module/players/factory.h>
#include <strings/map.h>

namespace Module::XSF
{
  using FilesMap = Strings::ValueMap<File>;

  class Factory
  {
  public:
    using Ptr = std::shared_ptr<const Factory>;
    virtual ~Factory() = default;

    virtual Holder::Ptr CreateSinglefileModule(const File& file, Parameters::Container::Ptr properties) const = 0;
    virtual Holder::Ptr CreateMultifileModule(const File& file, const FilesMap& additionalFiles,
                                              Parameters::Container::Ptr properties) const = 0;
  };

  Module::Factory::Ptr CreateFactory(XSF::Factory::Ptr delegate);
}  // namespace Module::XSF
