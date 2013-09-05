/*
Abstract:
  Parameters container factory

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//library includes
#include <parameters/container.h>
//std includes
#include <map>
//boost includes
#include <boost/make_shared.hpp>

namespace
{
  using namespace Parameters;

  template<class T>
  bool FindByName(const std::map<NameType, T>& map, const NameType& name, T& res)
  {
    const typename std::map<NameType, T>::const_iterator it = map.find(name);
    return it != map.end()
      ? (res = it->second, true)
      : false;
  }

  class ContainerImpl : public Container
  {
  public:
    ContainerImpl()
      : VersionValue(0)
    {
    }

    //accessor virtuals
    virtual uint_t Version() const
    {
      return VersionValue;
    }

    virtual bool FindValue(const NameType& name, IntType& val) const
    {
      return FindByName(Integers, name, val);
    }

    virtual bool FindValue(const NameType& name, StringType& val) const
    {
      return FindByName(Strings, name, val);
    }

    virtual bool FindValue(const NameType& name, DataType& val) const
    {
      return FindByName(Datas, name, val);
    }

    virtual void Process(Visitor& visitor) const
    {
      for (IntegerMap::const_iterator it = Integers.begin(), lim = Integers.end(); it != lim; ++it)
      {
        visitor.SetValue(it->first, it->second);
      }
      for (StringMap::const_iterator it = Strings.begin(), lim = Strings.end(); it != lim; ++it)
      {
        visitor.SetValue(it->first, it->second);
      }
      for (DataMap::const_iterator it = Datas.begin(), lim = Datas.end(); it != lim; ++it)
      {
        visitor.SetValue(it->first, it->second);
      }
    }

    //visitor virtuals
    virtual void SetValue(const NameType& name, IntType val)
    {
      if (Set(Integers[name], val) | Strings.erase(name) | Datas.erase(name))
      {
        ++VersionValue;
      }
    }

    virtual void SetValue(const NameType& name, const StringType& val)
    {
      if (Integers.erase(name) | Set(Strings[name], val) | Datas.erase(name))
      {
        ++VersionValue;
      }
    }

    virtual void SetValue(const NameType& name, const DataType& val)
    {
      if (Integers.erase(name) | Strings.erase(name) | Set(Datas[name], val))
      {
        ++VersionValue;
      }
    }

    //modifier virtuals
    virtual void RemoveValue(const NameType& name)
    {
      if (Integers.erase(name) | Strings.erase(name) | Datas.erase(name))
      {
        ++VersionValue;
      }
    }
  private:
    template<class Type>
    static std::size_t Set(Type& dst, const Type& src)
    {
      if (dst != src)
      {
        dst = src;
        return 1;
      }
      else
      {
        return 0;
      }
    }

    static std::size_t Set(DataType& dst, const DataType& src)
    {
      dst = src;
      return 1;
    }
  private:
    uint_t VersionValue;
    typedef std::map<NameType, IntType> IntegerMap;
    typedef std::map<NameType, StringType> StringMap;
    typedef std::map<NameType, DataType> DataMap;
    IntegerMap Integers;
    StringMap Strings;
    DataMap Datas;
  };
}

namespace Parameters
{
  Container::Ptr Container::Create()
  {
    return boost::make_shared<ContainerImpl>();
  }
}
