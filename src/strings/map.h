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

// common includes
#include <types.h>
// std includes
#include <map>
#include <stdexcept>

namespace Strings
{
  typedef std::map<String, String> Map;

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

    T& operator[](StringView key)
    {
      const auto it = find(key);
      if (it != end())
      {
        return it->second;
      }
      return emplace(key.to_string(), T()).first->second;
    }

    T& at(StringView key)
    {
      const auto it = find(key);
      if (it == end())
      {
        throw std::out_of_range("No key " + key.to_string());
      }
      return it->second;
    }

    const T& at(StringView key) const
    {
      const auto it = find(key);
      if (it == cend())
      {
        throw std::out_of_range("No key " + key.to_string());
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

    const T* FindPtr(StringView key) const
    {
      const auto it = find(key);
      return it != cend() ? &(it->second) : nullptr;
    }

    T Get(StringView key, T def = T()) const
    {
      const auto it = find(key);
      return it != cend() ? it->second : def;
    }
  };
}  // namespace Strings
