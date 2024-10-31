/**
 *
 * @file
 *
 * @brief  Simple strings map typedef
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "string_type.h"
#include "string_view.h"

#include <map>
#include <stdexcept>

namespace Strings
{
  using Map = std::map<String, String, std::less<>>;

  template<class T>
  class ValueMap : public std::map<String, T, std::less<>>
  {
    using parent = std::map<String, T, std::less<>>;

  public:
    using parent::parent;
    using parent::begin;
    using parent::end;
    using parent::cbegin;
    using parent::cend;
    using parent::size;
    using parent::find;
    using parent::insert;
    using parent::emplace;
    using parent::erase;

    // TODO: remove at c++26
    auto& operator[](StringView key)
    {
      const auto it = find(key);
      if (it != end())
      {
        return it->second;
      }
      return emplace(key, T()).first->second;
    }

    // TODO: remove at c++26
    auto& at(StringView key)
    {
      const auto it = find(key);
      if (it == end())
      {
        throw std::out_of_range("No key "s + key);
      }
      return it->second;
    }

    // TODO: remove at c++26
    const auto& at(StringView key) const
    {
      const auto it = find(key);
      if (it == cend())
      {
        throw std::out_of_range("No key "s + key);
      }
      return it->second;
    }

    // TODO: remove at c++23
    auto erase(StringView key)
    {
      const auto it = find(key);
      if (it != end())
      {
        parent::erase(it);
        return 1;
      }
      else
      {
        return 0;
      }
    }

    const auto* FindPtrValue(StringView key) const
    {
      const auto it = find(key);
      return it != cend() ? &*(it->second) : nullptr;
    }

    const auto* FindPtr(StringView key) const
    {
      const auto it = find(key);
      return it != cend() ? &(it->second) : nullptr;
    }

    auto Get(StringView key, T def = T()) const
    {
      const auto it = find(key);
      return it != cend() ? it->second : def;
    }
  };
}  // namespace Strings
