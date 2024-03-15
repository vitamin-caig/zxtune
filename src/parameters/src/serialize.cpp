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
}  // namespace

namespace Parameters
{
  void Convert(StringView name, StringView value, class Visitor& visitor)
  {
    if (const auto asInt = ConvertIntegerFromString(value))
    {
      visitor.SetValue(name, *asInt);
    }
    else if (const auto asData = ConvertDataFromString(value))
    {
      visitor.SetValue(name, *asData);
    }
    else if (const auto asString = ConvertStringFromString(value))
    {
      visitor.SetValue(name, *asString);
    }
  }

  void Convert(const Strings::Map& map, Visitor& visitor)
  {
    for (const auto& entry : map)
    {
      Convert(entry.first, entry.second, visitor);
    }
  }

  void Convert(const Accessor& ac, Strings::Map& strings)
  {
    StringConvertor cnv;
    ac.Process(cnv);
    cnv.swap(strings);
  }
}  // namespace Parameters
