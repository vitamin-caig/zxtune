/**
 *
 * @file
 *
 * @brief  XSF-based chiptune factories
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/players/xsf/xsf_factory.h"

namespace Module::XSF
{
  Holder::Ptr CreateGSFModule(const File& file, const FilesMap& additionalFiles, Parameters::Container::Ptr properties);
  Holder::Ptr CreateNCSFModule(const File& file, const FilesMap& additionalFiles,
                               Parameters::Container::Ptr properties);
  Holder::Ptr CreatePSFModule(const File& file, const FilesMap& additionalFiles, Parameters::Container::Ptr properties);
  Holder::Ptr CreateSDSFModule(const File& file, const FilesMap& additionalFiles,
                               Parameters::Container::Ptr properties);
  Holder::Ptr Create2SFModule(const File& file, const FilesMap& additionalFiles, Parameters::Container::Ptr properties);
  Holder::Ptr CreateUSFModule(const File& file, const FilesMap& additionalFiles, Parameters::Container::Ptr properties);
}  // namespace Module::XSF