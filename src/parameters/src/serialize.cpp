/**
*
* @file
*
* @brief  Serialization-related implementation
*
* @author vitamin.caig@gmail.com
*
**/

//library includes
#include <parameters/accessor.h>
#include <parameters/convert.h>
#include <parameters/serialize.h>
#include <parameters/visitor.h>
//boost includes
#include <boost/bind.hpp>
#include <boost/ref.hpp>

namespace
{
  using namespace Parameters;

  class StringConvertor : public Strings::Map
                        , public Visitor
  {
  public:
    virtual void SetValue(const NameType& name, IntType val)
    {
      insert(value_type(name.FullPath(), ConvertToString(val)));
    }

    virtual void SetValue(const NameType& name, const StringType& val)
    {
      insert(value_type(name.FullPath(), ConvertToString(val)));
    }

    virtual void SetValue(const NameType& name, const DataType& val)
    {
      insert(value_type(name.FullPath(), ConvertToString(val)));
    }
  };

  void SetValue(Visitor& visitor, const Strings::Map::value_type& pair)
  {
    const NameType& name = pair.first;
    const String& val = pair.second;
    IntType asInt;
    DataType asData;
    StringType asString;
    if (ConvertFromString(val, asInt))
    {
      visitor.SetValue(name, asInt);
    }
    else if (ConvertFromString(val, asData))
    {
      visitor.SetValue(name, asData);
    }
    else if (ConvertFromString(val, asString))
    {
      visitor.SetValue(name, asString);
    }
  }
}

namespace Parameters
{
  void Convert(const Strings::Map& map, Visitor& visitor)
  {
    std::for_each(map.begin(), map.end(), 
      boost::bind(&SetValue, boost::ref(visitor), _1));
  }

  void Convert(const Accessor& ac, Strings::Map& strings)
  {
    StringConvertor cnv;
    ac.Process(cnv);
    cnv.swap(strings);
  }
}
