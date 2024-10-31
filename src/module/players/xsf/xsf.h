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

#include "module/players/xsf/xsf_file.h"
#include "module/players/xsf/xsf_metainformation.h"

#include <formats/chiptune.h>

#include <string_view.h>

namespace Module::XSF
{
  Formats::Chiptune::Container::Ptr Parse(const Binary::Container& rawData, File& file);
  Formats::Chiptune::Container::Ptr Parse(StringView name, const Binary::Container& data, File& file);
}  // namespace Module::XSF
