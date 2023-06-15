/**
 *
 * @file
 *
 * @brief  Namespace-typed identifiers support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <string_view.h>
#include <types.h>

namespace Parameters
{
  class Identifier
  {
  public:
    /*explicit*/ constexpr Identifier(StringView sv)
      : Storage(sv)
    {}

    /*explicit*/ constexpr Identifier(const char* str)
      : Storage(str)
    {}

    /*explicit*/ constexpr Identifier(const String& str)
      : Storage(str)
    {}

    constexpr operator StringView() const
    {
      return Storage;
    }

    constexpr bool operator==(Identifier rh) const
    {
      return Storage == rh.Storage;
    }

    constexpr bool operator==(StringView rh) const
    {
      return Storage == rh;
    }

    constexpr bool IsEmpty() const
    {
      return Storage.empty();
    }

    constexpr bool IsPath() const
    {
      return StringView::npos != Storage.find(NAMESPACE_DELIMITER);
    }

    constexpr Identifier RelativeTo(Identifier rh) const
    {
      if (Storage.size() > rh.Storage.size() && 0 == Storage.compare(0, rh.Storage.size(), rh)
          && NAMESPACE_DELIMITER == Storage[rh.Storage.size()])
      {
        return Storage.substr(rh.Storage.size() + 1);
      }
      else
      {
        return {};
      }
    }

    StringView Name() const
    {
      const auto pos = Storage.find_last_of(NAMESPACE_DELIMITER);
      return pos != StringView::npos ? Storage.substr(pos + 1) : Storage;
    }

    String Append(Identifier rh) const
    {
      return rh.IsEmpty() ? AsString() : (IsEmpty() ? rh.AsString() : AsString() + NAMESPACE_DELIMITER + rh.AsString());
    }

    String AsString() const
    {
      return Storage.to_string();
    }

  private:
    constexpr Identifier()
      : Storage()
    {}

    constexpr Identifier(const Char* str, std::size_t size)
      : Storage(str, size)
    {}

  private:
    constexpr static const Char NAMESPACE_DELIMITER = '.';
    template<Char...>
    friend class StaticIdentifier;
    const StringView Storage;
  };

  template<Char... Symbols>
  class StaticIdentifier
  {
  public:
    constexpr operator Identifier() const
    {
      return {Storage.data(), Storage.size()};
    }

    template<Char... AnotherSymbols>
    constexpr auto operator+(const StaticIdentifier<AnotherSymbols...>&) const
    {
      static_assert(sizeof...(Symbols) * sizeof...(AnotherSymbols) != 0, "Empty components");
      return StaticIdentifier<Symbols..., Identifier::NAMESPACE_DELIMITER, AnotherSymbols...>{};
    }

  private:
    constexpr static const std::array<Char, sizeof...(Symbols)> Storage = {Symbols...};
  };

  template<typename CharType, CharType... Symbols>
  constexpr auto operator"" _id()
  {
    return StaticIdentifier<Symbols...>{};
  }
}  // namespace Parameters
