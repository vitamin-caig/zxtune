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

#include "strings/map.h"

namespace Module::XSF
{
  using FilesMap = Strings::ValueMap<File>;

  using Factory = Holder::Ptr (*)(const File& file, const FilesMap& additionalFiles,
                                  Parameters::Container::Ptr properties);

  Module::Factory::Ptr CreateModuleFactory(Factory create);
}  // namespace Module::XSF
