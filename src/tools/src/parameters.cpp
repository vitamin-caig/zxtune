/*
Abstract:
  Parameters-related functions implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//common includes
#include <parameters.h>
//std includes
#include <algorithm>
#include <cctype>
#include <functional>
#include <set>
//boost includes
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>

namespace
{
  using namespace Parameters;

  const IntType RADIX = 10;

  BOOST_STATIC_ASSERT(1 == sizeof(DataType::value_type));

  inline bool DoTest(const String::const_iterator it, const String::const_iterator lim, int(*Fun)(int))
  {
    return lim == std::find_if(it, lim, std::not1(std::ptr_fun(Fun)));
  }

  inline bool IsData(const String& str)
  {
    return str.size() >= 3 && DATA_PREFIX == *str.begin() && 0 == (str.size() - 1) % 2 &&
      DoTest(str.begin() + 1, str.end(), &std::isxdigit);
  }

  inline uint8_t FromHex(Char val)
  {
    assert(std::isxdigit(val));
    return val >= 'A' ? val - 'A' + 10 : val - '0';
  }

  inline void DataFromString(const String& val, DataType& res)
  {
    res.resize((val.size() - 1) / 2);
    String::const_iterator src = val.begin();
    for (DataType::iterator it = res.begin(), lim = res.end(); it != lim; ++it)
    {
      const DataType::value_type highNibble = FromHex(*++src);
      const DataType::value_type lowNibble = FromHex(*++src);
      *it = highNibble * 16 | lowNibble;
    }
  }

  inline Char ToHex(uint_t val)
  {
    assert(val < 16);
    return static_cast<Char>(val >= 10 ? val + 'A' - 10 : val + '0');
  }

  inline String DataToString(const DataType& dmp)
  {
    String res(dmp.size() * 2 + 1, DATA_PREFIX);
    String::iterator dstit = res.begin();
    for (DataType::const_iterator srcit = dmp.begin(), srclim = dmp.end(); srcit != srclim; ++srcit)
    {
      const DataType::value_type val = *srcit;
      *++dstit = ToHex(val >> 4);
      *++dstit = ToHex(val & 15);
    }
    return res;
  }

  inline bool IsInteger(const String& str)
  {
    return !str.empty() &&
      DoTest(str.begin() + (*str.begin() == '-' || *str.begin() == '+' ? 1 : 0), str.end(), &std::isdigit);
  }

  inline IntType IntegerFromString(const String& val)
  {
    IntType res = 0;
    String::const_iterator it = val.begin();
    const bool negate = *it == '-';
    if (negate || *it == '+')
    {
      ++it;
    }
    for (String::const_iterator lim = val.end(); it != lim; ++it)
    {
      res *= RADIX;
      res += *it - '0';
    }
    return negate ? -res : res;
  }

  inline String IntegerToString(IntType var)
  {
    //integer may be so long, so it's better to convert here
    String res;
    const bool negate = var < 0;

    if (negate)
    {
      var =- var;
    }
    do
    {
      res += ToHex(static_cast<uint_t>(var % RADIX));
    }
    while (var /= RADIX);

    if (negate)
    {
      res += '-';
    }
    return String(res.rbegin(), res.rend());
  }

  inline bool IsQuoted(const String& str)
  {
    return str.size() >= 2 && STRING_QUOTE == *str.begin()  && STRING_QUOTE == *str.rbegin();
  }
                    
  inline StringType StringFromString(const String& val)
  {
    if (IsQuoted(val))
    {
      return StringType(val.begin() + 1, val.end() - 1);
    }
    return val;
  }

  inline String StringToString(const StringType& str)
  {
    if (IsData(str) || IsInteger(str) || IsQuoted(str))
    {
      String res = String(1, STRING_QUOTE);
      res += str;
      res += STRING_QUOTE;
      return res;
    }
    return str;
  }

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
    {
    }

    //accessor virtuals
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
      Integers[name] = val;
      Strings.erase(name);
      Datas.erase(name);
    }

    virtual void SetValue(const NameType& name, const StringType& val)
    {
      Integers.erase(name);
      Strings[name] = val;
      Datas.erase(name);
    }

    virtual void SetValue(const NameType& name, const DataType& val)
    {
      Integers.erase(name);
      Strings.erase(name);
      Datas[name] = val;
    }

    //modifier virtuals
    virtual void RemoveValue(const NameType& name)
    {
      Integers.erase(name);
      Strings.erase(name);
      Datas.erase(name);
    }
  private:
    typedef std::map<NameType, IntType> IntegerMap;
    typedef std::map<NameType, StringType> StringMap;
    typedef std::map<NameType, DataType> DataMap;
    IntegerMap Integers;
    StringMap Strings;
    DataMap Datas;
  };

  class MergedVisitor : public Visitor
  {
  public:
    explicit MergedVisitor(Visitor& delegate)
      : Delegate(delegate)
    {
    }

    virtual void SetValue(const NameType& name, IntType val)
    {
      if (DoneIntegers.insert(name).second)
      {
        return Delegate.SetValue(name, val);
      }
    }

    virtual void SetValue(const NameType& name, const StringType& val)
    {
      if (DoneStrings.insert(name).second)
      {
        return Delegate.SetValue(name, val);
      }
    }

    virtual void SetValue(const NameType& name, const DataType& val)
    {
      if (DoneDatas.insert(name).second)
      {
        return Delegate.SetValue(name, val);
      }
    }
  private:
    Visitor& Delegate;
    std::set<NameType> DoneIntegers;
    std::set<NameType> DoneStrings;
    std::set<NameType> DoneDatas;
  };

  class DoubleAccessor : public Accessor
  {
  public:
    DoubleAccessor(Accessor::Ptr first, Accessor::Ptr second)
      : First(first)
      , Second(second)
    {
    }

    virtual bool FindValue(const NameType& name, IntType& val) const
    {
      return First->FindValue(name, val) || 
             Second->FindValue(name, val);
    }

    virtual bool FindValue(const NameType& name, StringType& val) const
    {
      return First->FindValue(name, val) || 
             Second->FindValue(name, val);
    }

    virtual bool FindValue(const NameType& name, DataType& val) const
    {
      return First->FindValue(name, val) || 
             Second->FindValue(name, val);
    }

    virtual void Process(Visitor& visitor) const
    {
      MergedVisitor mergedVisitor(visitor);
      First->Process(mergedVisitor);
      Second->Process(mergedVisitor);
    }
  private:
    const Accessor::Ptr First;
    const Accessor::Ptr Second;
  };

  class TripleAccessor : public Accessor
  {
  public:
    TripleAccessor(Accessor::Ptr first, Accessor::Ptr second, Accessor::Ptr third)
      : First(first)
      , Second(second)
      , Third(third)
    {
    }

    virtual bool FindValue(const NameType& name, IntType& val) const
    {
      return First->FindValue(name, val) || 
             Second->FindValue(name, val) ||
             Third->FindValue(name, val);
    }

    virtual bool FindValue(const NameType& name, StringType& val) const
    {
      return First->FindValue(name, val) || 
             Second->FindValue(name, val) ||
             Third->FindValue(name, val);
    }

    virtual bool FindValue(const NameType& name, DataType& val) const
    {
      return First->FindValue(name, val) || 
             Second->FindValue(name, val) ||
             Third->FindValue(name, val);
    }

    virtual void Process(Visitor& visitor) const
    {
      MergedVisitor mergedVisitor(visitor);
      First->Process(mergedVisitor);
      Second->Process(mergedVisitor);
      Third->Process(mergedVisitor);
    }
  private:
    const Accessor::Ptr First;
    const Accessor::Ptr Second;
    const Accessor::Ptr Third;
  };

  class StringConvertor : public Strings::Map
                        , public Visitor
  {
  public:
    virtual void SetValue(const NameType& name, IntType val)
    {
      insert(value_type(name.FullPath(), IntegerToString(val)));
    }

    virtual void SetValue(const NameType& name, const StringType& val)
    {
      insert(value_type(name.FullPath(), StringToString(val)));
    }

    virtual void SetValue(const NameType& name, const DataType& val)
    {
      insert(value_type(name.FullPath(), DataToString(val)));
    }
  };

  void SetValue(Visitor& visitor, const Strings::Map::value_type& pair)
  {
    const NameType& name = pair.first;
    const String& val = pair.second;
    if (IsData(val))
    {
      DataType res;
      DataFromString(val, res);
      visitor.SetValue(name, res);
    }
    else if (IsInteger(val))
    {
      const IntType res = IntegerFromString(val);
      visitor.SetValue(name, res);
    }
    else
    {
      visitor.SetValue(name, StringFromString(val));
    }
  }

  class PropertyTrackedContainer : public Container
  {
  public:
    PropertyTrackedContainer(Container::Ptr delegate, const PropertyChangedCallback& callback)
      : Delegate(delegate)
      , Callback(callback)
    {
    }

    virtual bool FindValue(const NameType& name, IntType& val) const
    {
      return Delegate->FindValue(name, val);
    }

    virtual bool FindValue(const NameType& name, StringType& val) const
    {
      return Delegate->FindValue(name, val);
    }

    virtual bool FindValue(const NameType& name, DataType& val) const
    {
      return Delegate->FindValue(name, val);
    }

    virtual void Process(Visitor& visitor) const
    {
      Delegate->Process(visitor);
    }

    virtual void SetValue(const NameType& name, IntType val)
    {
      Delegate->SetValue(name, val);
      Callback.OnPropertyChanged(name);
    }

    virtual void SetValue(const NameType& name, const StringType& val)
    {
      Delegate->SetValue(name, val);
      Callback.OnPropertyChanged(name);
    }

    virtual void SetValue(const NameType& name, const DataType& val)
    {
      Delegate->SetValue(name, val);
      Callback.OnPropertyChanged(name);
    }

    virtual void RemoveValue(const NameType& name)
    {
      Delegate->RemoveValue(name);
      Callback.OnPropertyChanged(name);
    }
  private:
    const Container::Ptr Delegate;
    const PropertyChangedCallback& Callback;
  };
}

namespace Parameters
{
  String ConvertToString(const IntType& val)
  {
    return IntegerToString(val);
  }

  String ConvertToString(const StringType& val)
  {
    return StringToString(val);
  }

  String ConvertToString(const DataType& val)
  {
    return DataToString(val);
  }

  bool ConvertFromString(const String& str, IntType& res)
  {
    if (IsInteger(str))
    {
      res = IntegerFromString(str);
      return true;
    }
    return false;
  }

  bool ConvertFromString(const String& str, StringType& res)
  {
    if (!IsInteger(str) && !IsData(str))
    {
      res = StringFromString(str);
      return true;
    }
    return false;
  }

  bool ConvertFromString(const String& str, DataType& res)
  {
    if (IsData(str))
    {
      DataFromString(str, res);
      return true;
    }
    return false;
  }

  void ParseStringMap(const Strings::Map& map, Visitor& visitor)
  {
    std::for_each(map.begin(), map.end(), 
      boost::bind(&SetValue, boost::ref(visitor), _1));
  }

  Accessor::Ptr CreateMergedAccessor(Accessor::Ptr first, Accessor::Ptr second)
  {
    return boost::make_shared<DoubleAccessor>(first, second);
  }

  Accessor::Ptr CreateMergedAccessor(Accessor::Ptr first, Accessor::Ptr second, Accessor::Ptr third)
  {
    return boost::make_shared<TripleAccessor>(first, second, third);
  }

  Container::Ptr Container::Create()
  {
    return boost::make_shared<ContainerImpl>();
  }

  void Convert(const Accessor& ac, Strings::Map& strings)
  {
    StringConvertor cnv;
    ac.Process(cnv);
    cnv.swap(strings);
  }

  Container::Ptr CreatePropertyTrackedContainer(Container::Ptr delegate, const PropertyChangedCallback& callback)
  {
    return boost::make_shared<PropertyTrackedContainer>(delegate, callback);
  }
}
