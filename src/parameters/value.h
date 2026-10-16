/**
 *
 * @file
 *
 * @brief  Typed parameters values support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "parameters/accessor.h"
#include "parameters/modifier.h"
#include "parameters/types.h"

#include "string_view.h"

#include <optional>
#include <type_traits>
#include <utility>

namespace Parameters
{
  namespace Detail
  {
    //! @brief Integer storage: optional IntType
    struct IntegerProtocol
    {
      static constexpr bool SupportsDefault = true;

      template<class P>
      static std::optional<typename P::Type> Find(const Accessor& ac)
      {
        if (const auto val = ac.FindInteger(P::Key))
        {
          return static_cast<typename P::Type>(*val);
        }
        return std::nullopt;
      }
    };

    //! @brief String storage: optional StringType
    struct StringProtocol
    {
      static constexpr bool SupportsDefault = true;

      template<class P>
      static auto Find(const Accessor& ac)
      {
        return ac.FindString(P::Key);
      }
    };

    //! @brief Binary data storage: Binary::Data::Ptr
    struct DataProtocol
    {
      static constexpr bool SupportsDefault = false;

      template<class P>
      static auto Find(const Accessor& ac)
      {
        return ac.FindData(P::Key);
      }
    };

    template<class Wire>
    using ProtocolOf = std::conditional_t<
        std::is_same_v<Wire, StringType>, StringProtocol,
        std::conditional_t<std::is_integral_v<Wire> || std::is_enum_v<Wire>, IntegerProtocol, DataProtocol>>;

  }  // namespace Detail

  //! @brief Base for a typed parameter descriptor; Wire defines both payload type and storage protocol
  template<class Wire>
  struct Value
  {
    using Type = Wire;
    using Protocol = Detail::ProtocolOf<Wire>;
  };

  //! @brief Interface of a typed parameter descriptor
  template<class P>
  concept Parameter = requires
  {
    P::Key;
    typename P::Type;
    typename P::Protocol;
  };

  //! @brief Parameter with an accessible default; data parameters are excluded on purpose
  template<class P>
  concept Defaulted = Parameter<P>&& P::Protocol::SupportsDefault&& requires
  {
    P::Default;
    requires std::is_convertible_v<decltype(P::Default), typename P::Type>;
  };

  //! @brief Find typed value; data parameters return a nullable pointer instead of an optional
  template<Parameter P>
  auto Find(const Accessor& ac)
  {
    return P::Protocol::template Find<P>(ac);
  }

  //! @brief Get typed value or the descriptor default; not available for data parameters
  template<Defaulted P>
  typename P::Type Get(const Accessor& ac)
  {
    return Find<P>(ac).value_or(P::Default);
  }

  //! @brief Check value against descriptor range (if any); opt-in, not enforced by Get/Find
  template<Parameter P, class V>
  constexpr bool Validate(V val)
  {
    if constexpr (requires {
                    P::Min;
                    P::Max;
                  })
    {
      const auto typed = static_cast<typename P::Type>(val);
      return typed >= P::Min && typed <= P::Max;
    }
    else
    {
      return true;
    }
  }

  //! @brief Set value; integer inputs are encoded, views are passed through
  template<Parameter P, class V>
  void Set(Visitor& target, V&& val)
  {
    if constexpr (std::is_same_v<typename P::Protocol, Detail::IntegerProtocol>)
    {
      target.SetValue(P::Key, static_cast<IntType>(static_cast<typename P::Type>(std::forward<V>(val))));
    }
    else
    {
      target.SetValue(P::Key, std::forward<V>(val));
    }
  }

  //! @brief Remove value
  template<Parameter P>
  void Remove(Modifier& target)
  {
    target.RemoveValue(P::Key);
  }
}  // namespace Parameters
