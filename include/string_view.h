/**
*
* @file
*
* @brief  basic_string_view compatibility
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//std includes
#include <algorithm>
#include <array>
#include <string>
#include <stdexcept>

//TODO: use standard c++17
template<class C, class Traits = std::char_traits<C>>
class basic_string_view
{
public:
  using value_type = C;
  using const_pointer = const C*;
  using const_iterator = const C*;
  using iterator = const_iterator;
  using size_type = std::size_t;
  
  static const size_type npos = size_type(-1);

  basic_string_view()
    : start(nullptr)
    , finish(nullptr)
  {
  }
  
  basic_string_view(const basic_string_view&) = default;
  basic_string_view& operator = (const basic_string_view&) = default;
  
  basic_string_view(const C* str, size_type size)
    : start(str)
    , finish(str + size)
  {
  }
  
  basic_string_view(const C* str)
    : basic_string_view(str, Traits::length(str))
  {
  }
  
  //NOTE: not compatible
  basic_string_view(const C* begin, const C* end)
    : start(begin)
    , finish(end)
  {
  }

  template<std::size_t D>
  basic_string_view(const std::array<C, D>& str)
    : basic_string_view(str.data(), str.size())
  {
  }
  
  //NOTE: not compatible
  basic_string_view(const std::basic_string<C, Traits>& rh)
    : basic_string_view(rh.data(), rh.size())
  {
  }
  
  const_pointer data() const
  {
    return start;
  }
 
  const_iterator begin() const
  {
    return start;
  }
  
  const_iterator end() const
  {
    return finish;
  }
  
  bool empty() const
  {
    return start == finish;
  }
  
  size_type size() const
  {
    return finish - start;
  }
  
  basic_string_view<C, Traits> substr(size_type pos = 0, size_type count = npos) const
  {
    const auto newStart = start + pos;
    if (newStart > finish)
    {
      throw std::out_of_range("basic_string_view::substr");
    }
    const auto newSize = std::min<size_type>(count, finish - newStart);
    return basic_string_view<C, Traits>(newStart, newSize);
  }
  
  size_type find(C sym, size_type pos = 0) const
  {
    const auto begin = start + pos;
    if (begin >= finish)
    {
      return npos;
    }
    const auto res = std::find(begin, finish, sym);
    return res >= finish
      ? npos
      : res - start;
  }
  
  size_type find(basic_string_view rh, size_type pos = 0) const
  {
    const auto begin = start + pos;
    if (begin >= finish)
    {
      return npos;
    }
    const auto res = std::search(begin, finish, rh.begin(), rh.end());
    return res >= finish
      ? npos
      : res - start;
  }

  size_type find_first_of(basic_string_view rh, size_type pos = 0) const
  {
    for (auto it = start + pos; it < finish; ++it)
    {
      const auto res = std::find(rh.begin(), rh.end(), *it);
      if (res < rh.end())
      {
        return it - start;
      }
    }
    return npos;
  }

  size_type find_first_not_of(basic_string_view rh, size_type pos = 0) const
  {
    for (auto it = start + pos; it < finish; ++it)
    {
      const auto res = std::find(rh.begin(), rh.end(), *it);
      if (res >= rh.end())
      {
        return it - start;
      }
    }
    return npos;
  }
  
  C operator [] (size_type pos) const
  {
    return start[pos];
  }
  
  int compare(const basic_string_view& rh) const
  {
    const auto res = Traits::compare(data(), rh.data(), std::min(size(), rh.size()));
    if (res != 0)
    {
      return res;
    }
    else if (size() < rh.size())
    {
      return -1;
    }
    else if (size() > rh.size())
    {
      return 1;
    }
    else
    {
      return 0;
    }
  }
  
  int compare(size_type pos, size_type count, const basic_string_view& rh) const
  {
    return substr(pos, count).compare(rh);
  }
  
  bool operator == (const basic_string_view& rh) const
  {
    return 0 == compare(rh);
  }
  
  //NOTE: not compatible
  std::basic_string<C, Traits> to_string() const
  {
    return empty()
      ? std::basic_string<C, Traits>()
      : std::basic_string<C, Traits>(start, finish);
  }
private:
  const_iterator start;
  const_iterator finish;
};

template<class C, class Traits>
inline std::basic_ostream<C, Traits>& operator << (std::basic_ostream<C, Traits>& str, basic_string_view<C, Traits> view)
{
  //TODO: support padding and adjustfield
  if (!view.empty())
  {
    str.rdbuf()->sputn(view.data(), view.size());
  }
  return str;
}
