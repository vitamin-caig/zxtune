/**
*
* @file
*
* @brief  Parameters containers implementation
*
* @author vitamin.caig@gmail.com
*
**/

//common includes
#include <make_ptr.h>
//library includes
#include <parameters/container.h>
//std includes
#include <map>

namespace Parameters
{
  template<class T>
  bool FindByName(const std::map<NameType, T>& map, const NameType& name, T& res)
  {
    const typename std::map<NameType, T>::const_iterator it = map.find(name);
    return it != map.end()
      ? (res = it->second, true)
      : false;
  }

  class StorageContainer : public Container
  {
  public:
    StorageContainer()
      : VersionValue(0)
    {
    }

    //accessor virtuals
    uint_t Version() const override
    {
      return VersionValue;
    }

    bool FindValue(const NameType& name, IntType& val) const override
    {
      return FindByName(Integers, name, val);
    }

    bool FindValue(const NameType& name, StringType& val) const override
    {
      return FindByName(Strings, name, val);
    }

    bool FindValue(const NameType& name, DataType& val) const override
    {
      return FindByName(Datas, name, val);
    }

    void Process(Visitor& visitor) const override
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
    void SetValue(const NameType& name, IntType val) override
    {
      if (Set(Integers[name], val) | Strings.erase(name) | Datas.erase(name))
      {
        ++VersionValue;
      }
    }

    void SetValue(const NameType& name, const StringType& val) override
    {
      if (Integers.erase(name) | Set(Strings[name], val) | Datas.erase(name))
      {
        ++VersionValue;
      }
    }

    void SetValue(const NameType& name, const DataType& val) override
    {
      if (Integers.erase(name) | Strings.erase(name) | Set(Datas[name], val))
      {
        ++VersionValue;
      }
    }

    //modifier virtuals
    void RemoveValue(const NameType& name) override
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

  class CompositeContainer : public Container
  {
  public:
    CompositeContainer(Accessor::Ptr accessor, Modifier::Ptr modifier)
      : AccessDelegate(accessor)
      , ModifyDelegate(modifier)
    {
    }

    //accessor virtuals
    uint_t Version() const override
    {
      return AccessDelegate->Version();
    }

    bool FindValue(const NameType& name, IntType& val) const override
    {
      return AccessDelegate->FindValue(name, val);
    }

    bool FindValue(const NameType& name, StringType& val) const override
    {
      return AccessDelegate->FindValue(name, val);
    }

    bool FindValue(const NameType& name, DataType& val) const override
    {
      return AccessDelegate->FindValue(name, val);
    }

    void Process(Visitor& visitor) const override
    {
      return AccessDelegate->Process(visitor);
    }

    //visitor virtuals
    void SetValue(const NameType& name, IntType val) override
    {
      return ModifyDelegate->SetValue(name, val);
    }

    void SetValue(const NameType& name, const StringType& val) override
    {
      return ModifyDelegate->SetValue(name, val);
    }

    void SetValue(const NameType& name, const DataType& val) override
    {
      return ModifyDelegate->SetValue(name, val);
    }

    //modifier virtuals
    void RemoveValue(const NameType& name) override
    {
      return ModifyDelegate->RemoveValue(name);
    }
  private:
    const Accessor::Ptr AccessDelegate;
    const Modifier::Ptr ModifyDelegate;
  };

  Container::Ptr Container::Create()
  {
    return MakePtr<StorageContainer>();
  }

  Container::Ptr Container::CreateAdapter(Accessor::Ptr accessor, Modifier::Ptr modifier)
  {
    return MakePtr<CompositeContainer>(accessor, modifier);
  }
}
