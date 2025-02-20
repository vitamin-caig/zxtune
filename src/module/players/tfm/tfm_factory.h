/**
 *
 * @file
 *
 * @brief  TFM-based modules factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/players/tfm/tfm_chiptune.h"

#include "binary/container.h"
#include "parameters/container.h"

namespace Module::TFM
{
  class Factory
  {
  public:
    using Ptr = std::unique_ptr<const Factory>;
    virtual ~Factory() = default;

    virtual Chiptune::Ptr CreateChiptune(const Binary::Container& data,
                                         Parameters::Container::Ptr properties) const = 0;
  };
}  // namespace Module::TFM
