/*
Abstract:
  Parameters-related functions implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include <parameters.h>

#include <algorithm>
#include <cctype>
#include <functional>
#include <boost/bind.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>

namespace
{
  const Char DUMP_MARKER = '#';
  const Char STRING_MARKER = '\'';
  const Parameters::IntType RADIX(10);

  BOOST_STATIC_ASSERT(1 == sizeof(Parameters::DataType::value_type));

  inline bool IsDump(const String& str)
  {
    return str.size() >= 3 && DUMP_MARKER == *str.begin() && 0 == (str.size() - 1) % 2 &&
      str.end() == std::find_if(str.begin() + 1, str.end(), std::not1(std::ptr_fun<int, int>(&std::isxdigit)));
  }
  
  inline bool IsInteger(const String& str)
  {
    return !str.empty() &&
      str.end() == std::find_if(str.begin(), str.end(), std::not1(std::ptr_fun<int, int>(&std::isdigit)));
  }
  
  inline bool IsQuoted(const String& str)
  {
    return !str.empty() && STRING_MARKER == *str.begin()  && STRING_MARKER == *str.rbegin();
  }

  inline Char ToHex(unsigned val)
  {
    assert(val < 16);
    return val >= 10 ? val + 'A' - 10 : val + '0';
  }
  
  inline unsigned FromHex(Char val)
  {
    assert(std::isxdigit(val));
    return val >= 'A' ? val - 'A' + 10 : val - '0';
  }

  class AsStringVisitor : public boost::static_visitor<String>
  {
  public:
    String operator()(const Parameters::DataType& dmp) const
    {
      String res(dmp.size() * 2 + 1, DUMP_MARKER);
      String::iterator dstit(res.begin());
      for (Parameters::DataType::const_iterator srcit = dmp.begin(), srclim = dmp.end(); srcit != srclim; ++srcit)
      {
        const Parameters::DataType::value_type val = *srcit;
        *++dstit = ToHex(val >> 4);
        *++dstit = ToHex(val & 15);
      }
      return res;
    }
    
    String operator()(const Parameters::StringType& str) const
    {
      if (IsDump(str) || IsInteger(str) || IsQuoted(str))
      {
        return String(1, STRING_MARKER) + str + STRING_MARKER;
      }
      return str;
    }
    
    String operator()(Parameters::IntType var) const
    {
      //integer may be so long, so it's better to convert here
      String res;
      do
      {
        res += ToHex(static_cast<unsigned>(var % RADIX));
      }
      while (var /= RADIX);
      return String(res.rbegin(), res.rend());
    }
  };
}

namespace Parameters
{
  String ConvertToString(const ValueType& val)
  {
    return boost::apply_visitor(AsStringVisitor(), val);
  }

  ValueType ConvertFromString(const String& val)
  {
    if (IsDump(val))
    {
      Dump res((val.size() - 1) / 2);
      String::const_iterator src = val.begin();
      for (Dump::iterator it = res.begin(), lim = res.end(); it != lim; ++it)
      {
        const Dump::value_type highNibble = FromHex(*++src);
        const Dump::value_type lowNibble = FromHex(*++src);
        *it = (highNibble << 4) | lowNibble;
      }
      return ValueType(res);
    }
    else if (IsInteger(val))
    {
      IntType res = 0;
      for (String::const_iterator it = val.begin(), lim = val.end(); it != lim; ++it)
      {
        res *= RADIX;
        res += *it - '0';
      }
      return ValueType(res);
    }
    else
    {
      if (IsQuoted(val))
      {
        return ValueType(String(val.begin() + 1, val.end() - 1));
      }
      return ValueType(val);
    }
  }
  void ConvertMap(const Map& input, StringMap& output)
  {
    StringMap res;
    std::transform(input.begin(), input.end(), std::inserter(res, res.end()),
      boost::bind(&std::make_pair<const StringMap::key_type, StringMap::mapped_type>,
        boost::bind<Map::key_type>(&Map::value_type::first, _1),
        boost::bind(ConvertToString, boost::bind<Map::mapped_type>(&Map::value_type::second, _1))));
    output.swap(res);
  }
  
  void ConvertMap(const StringMap& input, Map& output)
  {
    Map res;
    std::transform(input.begin(), input.end(), std::inserter(res, res.end()),
      boost::bind(&std::make_pair<const Map::key_type, Map::mapped_type>,
        boost::bind<StringMap::key_type>(&StringMap::value_type::first, _1),
        boost::bind(ConvertFromString, boost::bind<StringMap::mapped_type>(&StringMap::value_type::second, _1))));
    
    output.swap(res);
  }
}
