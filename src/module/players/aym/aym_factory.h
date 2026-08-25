/**
 *
 * @file
 *
 * @brief  AYM-based modules factory declaration
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/players/aym/aym_chiptune.h"
#include "module/players/factory.h"

#include "binary/container.h"
#include "parameters/container.h"

namespace Module::AYM
{
  class Factory
  {
  public:
    using Ptr = std::unique_ptr<const Factory>;
    virtual ~Factory() = default;

    virtual Chiptune::Ptr CreateChiptune(const Binary::Container& data,
                                         Parameters::Container::Ptr properties) const = 0;
  };

  Module::Factory::Ptr CreateModuleFactory(Factory::Ptr delegate);
}  // namespace Module::AYM
