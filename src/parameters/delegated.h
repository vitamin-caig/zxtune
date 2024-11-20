/**
 *
 * @file
 *
 * @brief  Delegate helpers
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "parameters/identifier.h"
#include "parameters/types.h"

#include <memory>
#include <optional>

namespace Parameters
{
  template<class Base, class PtrType = typename Base::Ptr>
  class Delegated : public Base
  {
  public:
    uint_t Version() const override
    {
      return Delegate->Version();
    }

    std::optional<IntType> FindInteger(Identifier name) const override
    {
      return Delegate->FindInteger(name);
    }

    std::optional<StringType> FindString(Identifier name) const override
    {
      return Delegate->FindString(name);
    }

    Binary::Data::Ptr FindData(Identifier name) const override
    {
      return Delegate->FindData(name);
    }

    void Process(class Visitor& visitor) const override
    {
      return Delegate->Process(visitor);
    }

  protected:
    explicit Delegated(PtrType delegate)
      : Delegate(std::move(delegate))
    {}

  protected:
    const PtrType Delegate;
  };

  using DelegatedAccessor = Delegated<Accessor>;
}  // namespace Parameters
