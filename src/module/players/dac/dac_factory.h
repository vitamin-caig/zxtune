/**
 *
 * @file
 *
 * @brief  DAC-based chiptunes factory declaration
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// local includes
#include "module/players/dac/dac_chiptune.h"
// library includes
#include <binary/container.h>
#include <parameters/container.h>

namespace Module::DAC
{
  class Factory
  {
  public:
    using Ptr = std::shared_ptr<const Factory>;
    virtual ~Factory() = default;

    virtual Chiptune::Ptr CreateChiptune(const Binary::Container& data,
                                         Parameters::Container::Ptr properties) const = 0;
  };
}  // namespace Module::DAC
