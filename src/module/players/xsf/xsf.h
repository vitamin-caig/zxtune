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

// local includes
#include "module/players/xsf/xsf_file.h"
#include "module/players/xsf/xsf_metainformation.h"
// library includes
#include <formats/chiptune.h>

namespace Module::XSF
{
  Formats::Chiptune::Container::Ptr Parse(const Binary::Container& rawData, File& file);
  Formats::Chiptune::Container::Ptr Parse(StringView name, const Binary::Container& data, File& file);
}  // namespace Module::XSF
