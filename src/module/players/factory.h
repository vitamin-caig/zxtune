/**
 *
 * @file
 *
 * @brief  Module factory interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "binary/container.h"
#include "module/holder.h"
#include "parameters/container.h"

namespace Formats::Chiptune
{
  class Container;
}

namespace Formats::Multitrack
{
  class Container;
}

namespace Module
{
  template<class ContainerType>
  class BaseFactory
  {
  public:
    using Ptr = std::unique_ptr<const BaseFactory>;
    virtual ~BaseFactory() = default;

    virtual Holder::Ptr CreateModule(const Parameters::Accessor& params, const ContainerType& data,
                                     Parameters::Container::Ptr properties) const = 0;
  };

  using Factory = BaseFactory<Binary::Container>;
  using ExternalParsingFactory = BaseFactory<Formats::Chiptune::Container>;

  class MultitrackFactory : public BaseFactory<Formats::Multitrack::Container>
  {
  public:
    using Ptr = std::shared_ptr<const MultitrackFactory>;
  };
}  // namespace Module
