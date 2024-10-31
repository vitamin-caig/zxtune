#pragma once

#include <algorithm>
#include <functional>
#include <ostream>
#include <vector>

template<class T1, class T2, std::enable_if_t<!std::is_same_v<T1, T2>, int> = 0>
bool operator==(const std::vector<T1>& lh, const std::vector<T2>& rh)
{
  return lh.size() == rh.size() && lh.end() == std::mismatch(lh.begin(), lh.end(), rh.begin()).first;
}

template<class T>
std::ostream& operator<<(std::ostream& o, const std::vector<T>& v)
{
  if (v.empty())
  {
    o << "[]";
    return o;
  }
  char delimiter = '[';
  for (const auto& e : v)
  {
    o << delimiter;
    delimiter = ',';
    if constexpr (std::is_same_v<T, String> || std::is_same_v<T, StringView>)
    {
      o << '\'' << e << '\'';
    }
    else
    {
      o << e;
    }
  }
  o << ']';
  return o;
}
