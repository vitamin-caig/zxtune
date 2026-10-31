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
#include "time/duration.h"

#include "string_view.h"

#include <optional>
#include <type_traits>
#include <utility>

namespace Parameters
{
  namespace Detail
  {
    template<class T>
    struct IsDuration : std::false_type
    {};

    template<class Unit>
    struct IsDuration<Time::Duration<Unit>> : std::true_type
    {};

    //! @brief Integer storage: optional IntType
    struct IntegerProtocol
    {
      static constexpr bool SupportsDefault = true;

      template<class Wire>
      static constexpr bool Accepts = std::is_integral_v<Wire> || std::is_enum_v<Wire> || IsDuration<Wire>::value;

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
    using ProtocolOf =
        std::conditional_t<std::is_same_v<Wire, StringType>, StringProtocol,
                           std::conditional_t<IntegerProtocol::Accepts<Wire>, IntegerProtocol, DataProtocol>>;

  }  // namespace Detail

  //! @brief Convert typed value to its integer wire representation
  template<class T>
  constexpr IntType Encode(T val)
  {
    return static_cast<IntType>(val);
  }

  template<class Unit>
  constexpr IntType Encode(Time::Duration<Unit> val)
  {
    // raw unit count is the wire format
    return static_cast<IntType>(val.Get());
  }

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

  //! @brief Parameter with an explicit range contract
  template<class P>
  concept Ranged = Parameter<P>&& requires
  {
    P::Min;
    P::Max;
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

  //! @brief Check value against the descriptor range; not enforced by Get/Find
  template<Ranged P, class V>
  constexpr bool Validate(V val)
  {
    const auto typed = static_cast<typename P::Type>(val);
    return !(typed < P::Min) && !(P::Max < typed);
  }

  //! @brief Set value; integer inputs are encoded, views are passed through
  template<Parameter P, class V>
  void Set(Visitor& target, V&& val)
  {
    if constexpr (std::is_same_v<typename P::Protocol, Detail::IntegerProtocol>)
    {
      target.SetValue(P::Key, Encode(static_cast<typename P::Type>(std::forward<V>(val))));
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
