/**
 *
 * @file
 *
 * @brief  Fields source filter adapter
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <contract.h>
// library includes
#include <strings/fields.h>
// std includes
#include <set>
#include <type_traits>
#include <vector>

namespace Strings
{
  static_assert(std::is_unsigned<Char>::value, "Char type should be unsigned");

  class FilterFieldsSource : public FieldsSource
  {
  public:
    FilterFieldsSource(const FieldsSource& delegate, StringView src, StringView tgt)
      : Delegate(delegate)
      , Table(1 << (8 * sizeof(Char)))
    {
      Require(src.size() == tgt.size());
      Require(std::set<Char>(src.begin(), src.end()).size() == src.size());
      for (std::size_t idx = 0; idx != Table.size(); ++idx)
      {
        const String::size_type srcPos = src.find(idx);
        Table[idx] = srcPos != src.npos ? tgt[srcPos] : idx;
      }
    }

    FilterFieldsSource(const FieldsSource& delegate, StringView src, const Char tgt)
      : Delegate(delegate)
      , Table(1 << (8 * sizeof(Char)))
    {
      const std::set<Char> srcSymbols(src.begin(), src.end());
      Require(srcSymbols.size() == src.size());
      for (std::size_t idx = 0; idx != Table.size(); ++idx)
      {
        Table[idx] = srcSymbols.count(idx) ? tgt : idx;
      }
    }

    String GetFieldValue(StringView fieldName) const override
    {
      auto val = Delegate.GetFieldValue(fieldName);
      for (auto& it : val)
      {
        it = Table[it];
      }
      return val;
    }

  private:
    const FieldsSource& Delegate;
    std::vector<Char> Table;
  };
}  // namespace Strings
