/**
 *
 * @file
 *
 * @brief  Parameters conversion
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// library includes
#include <parameters/convert.h>
#include <strings/conversion.h>
// std includes
#include <cassert>
#include <cctype>

namespace
{
  using namespace Parameters;

  static_assert(1 == sizeof(DataType::value_type), "Invalid DataType::value_type");

  template<class It>
  inline bool DoTest(const It it, const It lim, int (*fun)(int))
  {
    return std::all_of(it, lim, fun);
  }

  inline bool IsData(StringView str)
  {
    return str.size() >= 3 && DATA_PREFIX == *str.begin() && 0 == (str.size() - 1) % 2
           && DoTest(str.begin() + 1, str.end(), &std::isxdigit);
  }

  inline uint8_t FromHex(Char val)
  {
    assert(std::isxdigit(val));
    return val >= 'A' ? val - 'A' + 10 : val - '0';
  }

  inline void DataFromString(StringView val, DataType& res)
  {
    res.resize((val.size() - 1) / 2);
    const auto* src = val.begin();
    for (auto& re : res)
    {
      const auto highNibble = FromHex(*++src);
      const auto lowNibble = FromHex(*++src);
      re = highNibble * 16 | lowNibble;
    }
  }

  inline Char ToHex(uint_t val)
  {
    assert(val < 16);
    return static_cast<Char>(val >= 10 ? val + 'A' - 10 : val + '0');
  }

  inline String DataToString(Binary::View dmp)
  {
    String res(dmp.Size() * 2 + 1, DATA_PREFIX);
    String::iterator dstit = res.begin();
    for (const auto *it = dmp.As<uint8_t>(), *lim = it + dmp.Size(); it != lim; ++it)
    {
      const auto val = *it;
      *++dstit = ToHex(val >> 4);
      *++dstit = ToHex(val & 15);
    }
    return res;
  }

  inline bool IsInteger(StringView str)
  {
    return !str.empty()
           && DoTest(str.begin() + (*str.begin() == '-' || *str.begin() == '+' ? 1 : 0), str.end(), &std::isdigit);
  }

  inline IntType IntegerFromString(StringView val)
  {
    return Strings::ConvertTo<IntType>(val);
  }

  inline String IntegerToString(IntType var)
  {
    return Strings::ConvertFrom(var);
  }

  inline bool IsQuoted(StringView str)
  {
    return str.size() >= 2 && STRING_QUOTE == *str.begin() && STRING_QUOTE == *str.rbegin();
  }

  inline StringView StringFromString(StringView val)
  {
    if (IsQuoted(val))
    {
      return {val.begin() + 1, val.end() - 1};
    }
    return val;
  }

  inline String StringToString(StringView str)
  {
    if (IsData(str) || IsInteger(str) || IsQuoted(str))
    {
      String res = String(1, STRING_QUOTE);
      res += str;
      res += STRING_QUOTE;
      return res;
    }
    return str.to_string();
  }
}  // namespace

namespace Parameters
{
  String ConvertToString(IntType val)
  {
    return IntegerToString(val);
  }

  String ConvertToString(StringView val)
  {
    return StringToString(val);
  }

  String ConvertToString(Binary::View val)
  {
    return DataToString(val);
  }

  bool ConvertFromString(StringView str, IntType& res)
  {
    if (IsInteger(str))
    {
      res = IntegerFromString(str);
      return true;
    }
    return false;
  }

  bool ConvertFromString(StringView str, StringType& res)
  {
    if (!IsInteger(str) && !IsData(str))
    {
      res = StringFromString(str);
      return true;
    }
    return false;
  }

  bool ConvertFromString(StringView str, DataType& res)
  {
    if (IsData(str))
    {
      DataFromString(str, res);
      return true;
    }
    return false;
  }
}  // namespace Parameters
