/**
 *
 * @file
 *
 * @brief  String split
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <types.h>
// boost includes
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

namespace Strings
{
  namespace Details
  {
    inline auto ToPredicate(Char c)
    {
      return boost::algorithm::is_from_range(c, c);
    }

    inline auto ToPredicate(StringView any)
    {
      return boost::algorithm::is_any_of(any);
    }

    template<class T>
    inline auto ToPredicate(T p)
    {
      return p;
    }
  }  // namespace Details

  template<class T, class D>
  void Split(StringView str, D delimiter, T& target)
  {
    boost::algorithm::split(target, str, Details::ToPredicate(delimiter), boost::algorithm::token_compress_on);
  }
}  // namespace Strings
