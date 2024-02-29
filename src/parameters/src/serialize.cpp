/**
 *
 * @file
 *
 * @brief  Serialization-related implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// library includes
#include <parameters/accessor.h>
#include <parameters/convert.h>
#include <parameters/serialize.h>
#include <parameters/visitor.h>
// std includes
#include <algorithm>

namespace
{
  using namespace Parameters;

  class StringConvertor
    : public Strings::Map
    , public Visitor
  {
  public:
    void SetValue(Identifier name, IntType val) override
    {
      emplace(name.AsString(), ConvertToString(val));
    }

    void SetValue(Identifier name, StringView val) override
    {
      emplace(name.AsString(), ConvertToString(val));
    }

    void SetValue(Identifier name, Binary::View val) override
    {
      emplace(name.AsString(), ConvertToString(val));
    }
  };

  void SetValue(Visitor& visitor, StringView name, StringView val)
  {
    IntType asInt;
    StringType asString;
    if (ConvertFromString(val, asInt))
    {
      visitor.SetValue(name, asInt);
    }
    else if (const auto asData = ConvertDataFromString(val))
    {
      visitor.SetValue(name, *asData);
    }
    else if (ConvertFromString(val, asString))
    {
      visitor.SetValue(name, asString);
    }
  }
}  // namespace

namespace Parameters
{
  void Convert(const Strings::Map& map, Visitor& visitor)
  {
    for (const auto& entry : map)
    {
      SetValue(visitor, entry.first, entry.second);
    }
  }

  void Convert(const Accessor& ac, Strings::Map& strings)
  {
    StringConvertor cnv;
    ac.Process(cnv);
    cnv.swap(strings);
  }
}  // namespace Parameters
