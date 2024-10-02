/**
 *
 * @file
 *
 * @brief  Source files location functionality
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <string_view.h>
#include <types.h>
// std includes
#include <array>

#ifndef DO_LITERAL
#  define DO_LITERAL_IMPL(str, lit) str##lit
#  define DO_LITERAL(str, lit) DO_LITERAL_IMPL(str, lit)
#endif

namespace Debug
{
  template<class T = uint64_t>
  constexpr T Hash(StringView str)
  {
    if (str.empty())
    {
      return 0;
    }
    else
    {
      T res = 5381;
      for (auto c : str)
      {
        res = ((res << 5) + res) + c;
      }
      return res;
    }
  }

  template<class C>
  constexpr auto IsSeparator(C sym)
  {
    return sym == '/' || sym == '\\';
  }

  template<class C>
  constexpr C ToLower(C sym)
  {
    // static_assert(sym >= ' ' && sym <= 127, "Non-ascii path");
    return sym >= 'A' && sym <= 'Z' ? sym + ('a' - 'A') : sym;
  }

  template<class C>
  constexpr C Normalize(C in)
  {
    return in == '\\' ? '/' : ToLower(in);
  }

  template<class C>
  constexpr auto MatchesPathElement(C lh, C rh)
  {
    return (IsSeparator(lh) && IsSeparator(rh)) || (ToLower(lh) == ToLower(rh));
  }
}  // namespace Debug

template<class C, C... Chars>
class SourceFile
{
  static_assert(sizeof...(Chars) != 0, "Empty SourceFile");
  constexpr static const std::array<C, sizeof...(Chars)> Path{Debug::Normalize(Chars)...};

public:
  using TagType = uint32_t;

  constexpr static const auto StaticLocation = std::basic_string_view<C>{Path.data(), Path.size()};
  constexpr static const auto StaticTag = Debug::Hash<TagType>(StaticLocation);

  constexpr auto Location() const
  {
    return StaticLocation;
  }

  constexpr auto Tag() const
  {
    return StaticTag;
  }

  class SourceLine
  {
  public:
    constexpr explicit SourceLine(uint_t value)
      : Value(value)
    {}

    constexpr auto Location() const
    {
      return StaticLocation;
    }

    constexpr TagType Tag() const
    {
      return StaticTag + Value;
    }

    constexpr auto Line() const
    {
      return Value;
    }

  private:
    const uint_t Value;
  };

  constexpr auto Line(uint_t value) const
  {
    return SourceLine(value);
  }
};

namespace Debug
{
#ifdef SOURCES_ROOT
#  define STR(a) #  a
  constexpr auto ROOT = DO_LITERAL(SOURCES_ROOT, sv);
#  undef STR
#else
  constexpr auto ROOT = ""sv;
#endif
  template<std::size_t Offset, typename C, C Head, C... Tail>
  constexpr auto MakeSourceFile()
  {
    if constexpr (Offset == ROOT.size())
    {
      static_assert(Head == '/' || Head == '\\', "SOURCES_ROOT should not end with slash");
      return SourceFile<C, Tail...>();
    }
    else
    {
      static_assert(MatchesPathElement(Head, ROOT[Offset]), "Path outside of SOURCES_ROOT");
      return MakeSourceFile<Offset + 1, C, Tail...>();
    }
  }
}  // namespace Debug

template<typename C, C... Chars>
constexpr auto operator"" _source()
{
  return Debug::MakeSourceFile<0, C, Chars...>();
}

#define ThisFile() DO_LITERAL(__FILE__, _source)
#define ThisLine() ThisFile().Line(__LINE__)
