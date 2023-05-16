/**
 *
 * @file
 *
 * @brief  multidevice-based chiptunes support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// library includes
#include <module/holder.h>
// std includes
#include <vector>

namespace Module::Multi
{
  using HoldersArray = std::vector<Module::Holder::Ptr>;

  // first holder is used as a main one
  Module::Holder::Ptr CreateHolder(Parameters::Accessor::Ptr tuneProperties, HoldersArray holders);
}  // namespace Module::Multi
