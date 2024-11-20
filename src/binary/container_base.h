/**
 *
 * @file
 *
 * @brief  Binary::Container CRTP helper
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "binary/container.h"

#include <type_traits>

namespace Binary
{
  template<class BaseType, class DelegateType = Container>
  class BaseContainer : public BaseType
  {
  public:
    explicit BaseContainer(typename DelegateType::Ptr delegate)
      : Delegate(std::move(delegate))
    {
      static_assert(std::is_base_of<Container, BaseType>::value,
                    "Template parameter should have Binary::Container base");
      static_assert(std::is_base_of<Container, DelegateType>::value,
                    "Template parameter should have Binary::Container base");
    }

    const void* Start() const override
    {
      return Delegate->Start();
    }

    std::size_t Size() const override
    {
      return Delegate->Size();
    }

    Binary::Container::Ptr GetSubcontainer(std::size_t offset, std::size_t size) const override
    {
      return Delegate->GetSubcontainer(offset, size);
    }

  protected:
    const typename DelegateType::Ptr Delegate;
  };
}  // namespace Binary
