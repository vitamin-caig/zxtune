/**
 *
 * @file
 *
 * @brief  Xsf-based files structure support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "formats/chiptune.h"

#include "string_view.h"

namespace Module::XSF
{
  struct File;

  Formats::Chiptune::Container::Ptr Parse(const Binary::Container& rawData, File& file);
  Formats::Chiptune::Container::Ptr Parse(StringView name, const Binary::Container& data, File& file);
}  // namespace Module::XSF
