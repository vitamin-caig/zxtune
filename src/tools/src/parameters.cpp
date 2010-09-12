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
//boost includes
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>

namespace
{
  const Parameters::IntType RADIX = 10;

  BOOST_STATIC_ASSERT(1 == sizeof(Parameters::DataType::value_type));

  inline bool DoTest(const String::const_iterator it, const String::const_iterator lim, int(*Fun)(int))
  {
    return lim == std::find_if(it, lim, std::not1(std::ptr_fun(Fun)));
  }

  inline bool IsDump(const String& str)
  {
    return str.size() >= 3 && Parameters::DATA_PREFIX == *str.begin() && 0 == (str.size() - 1) % 2 &&
      DoTest(str.begin() + 1, str.end(), &std::isxdigit);
  }

  inline bool IsInteger(const String& str)
  {
    return !str.empty() &&
      DoTest(str.begin() + (*str.begin() == '-' || *str.begin() == '+' ? 1 : 0), str.end(), &std::isdigit);
  }

  inline bool IsQuoted(const String& str)
  {
    return !str.empty() && Parameters::STRING_QUOTE == *str.begin()  && Parameters::STRING_QUOTE == *str.rbegin();
  }

  inline Char ToHex(uint_t val)
  {
    assert(val < 16);
    return static_cast<Char>(val >= 10 ? val + 'A' - 10 : val + '0');
  }

  inline uint8_t FromHex(Char val)
  {
    assert(std::isxdigit(val));
    return val >= 'A' ? val - 'A' + 10 : val - '0';
  }

  class AsStringVisitor : public boost::static_visitor<String>
  {
  public:
    String operator()(const Parameters::DataType& dmp) const
    {
      String res(dmp.size() * 2 + 1, Parameters::DATA_PREFIX);
      String::iterator dstit = res.begin();
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
        String res = String(1, Parameters::STRING_QUOTE);
        res += str;
        res += Parameters::STRING_QUOTE;
        return res;
      }
      return str;
    }

    String operator()(Parameters::IntType var) const
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
  };

  inline bool CompareParameter(const Parameters::Map::value_type& lh, const Parameters::Map::value_type& rh)
  {
    return lh.first == rh.first ? !(lh.second == rh.second) : lh.first < rh.first;
  }

  template<class T>
  bool FindByName(const std::map<Parameters::NameType, T>& map, const Parameters::NameType& name, T& res)
  {
    const typename std::map<Parameters::NameType, T>::const_iterator it = map.find(name);
    return it != map.end()
      ? (res = it->second, true)
      : false;
  }

  using namespace Parameters;
  class ContainerImpl : public Container
  {
  public:
    ContainerImpl()
    {
    }

    //accessor virtuals
    virtual bool FindIntValue(const NameType& name, IntType& val) const
    {
      return FindByName(Integers, name, val);
    }

    virtual bool FindStringValue(const NameType& name, StringType& val) const
    {
      return FindByName(Strings, name, val);
    }

    virtual bool FindDataValue(const NameType& name, DataType& val) const
    {
      return FindByName(Datas, name, val);
    }

    virtual void Process(Visitor& visitor) const
    {
      std::for_each(Integers.begin(), Integers.end(), boost::bind(&Visitor::SetIntValue, &visitor,
        boost::bind(&IntegerMap::value_type::first, _1), boost::bind(&IntegerMap::value_type::second, _1)));
      std::for_each(Strings.begin(), Strings.end(), boost::bind(&Visitor::SetStringValue, &visitor,
        boost::bind(&StringMap::value_type::first, _1), boost::bind(&StringMap::value_type::second, _1)));
      std::for_each(Datas.begin(), Datas.end(), boost::bind(&Visitor::SetDataValue, &visitor,
        boost::bind(&DataMap::value_type::first, _1), boost::bind(&DataMap::value_type::second, _1)));
    }

    //modifier virtuals
    virtual void SetIntValue(const NameType& name, IntType val)
    {
      Integers.insert(IntegerMap::value_type(name, val));
    }

    virtual void SetStringValue(const NameType& name, const StringType& val)
    {
      Strings.insert(StringMap::value_type(name, val));
    }

    virtual void SetDataValue(const NameType& name, const DataType& val)
    {
      Datas.insert(DataMap::value_type(name, val));
    }

  private:
    typedef std::map<NameType, IntType> IntegerMap;
    typedef std::map<NameType, StringType> StringMap;
    typedef std::map<NameType, DataType> DataMap;
    IntegerMap Integers;
    StringMap Strings;
    DataMap Datas;
  };

  class MergedAccessor : public Accessor
  {
    static void MergeString(String& lh, const String& rh)
    {
      if (lh != rh)
      {
        lh += '/';
        lh += rh;
      }
    }

    class MergedStringsVisitor : public Visitor
    {
    public:
      explicit MergedStringsVisitor(Visitor& delegate)
        : Delegate(delegate)
      {
      }

      virtual void SetIntValue(const NameType& name, IntType val)
      {
        return Delegate.SetIntValue(name, val);
      }

      virtual void SetStringValue(const NameType& name, const StringType& val)
      {
        const StringMap::iterator it = Strings.find(name);
        if (it == Strings.end())
        {
          Strings.insert(StringMap::value_type(name, val));
        }
        else
        {
          MergeString(it->second, val);
        }
      }

      virtual void SetDataValue(const NameType& name, const DataType& val)
      {
        return Delegate.SetDataValue(name, val);
      }

      void ProcessRestStrings() const
      {
        std::for_each(Strings.begin(), Strings.end(), boost::bind(&Visitor::SetStringValue, &Delegate,
          boost::bind(&StringMap::value_type::first, _1), boost::bind(&StringMap::value_type::second, _1)));
      }
    private:
      Visitor& Delegate;
      StringMap Strings;
    };
  public:
    MergedAccessor(Accessor::Ptr first, Accessor::Ptr second)
      : First(first)
      , Second(second)
    {
    }

    virtual bool FindIntValue(const NameType& name, IntType& val) const
    {
      return First->FindIntValue(name, val) || Second->FindIntValue(name, val);
    }

    virtual bool FindStringValue(const NameType& name, StringType& val) const
    {
      String val1, val2;
      const bool res1 = First->FindStringValue(name, val1);
      const bool res2 = Second->FindStringValue(name, val2);
      if (res1 && res2)
      {
        MergeString(val1, val2);
        val = val1;
      }
      else if (res1 != res2)
      {
        val = res1 ? val1 : val2;
      }
      return res1 || res2;
    }

    virtual bool FindDataValue(const NameType& name, DataType& val) const
    {
      return First->FindDataValue(name, val) || Second->FindDataValue(name, val);
    }

    virtual void Process(Visitor& visitor) const
    {
      MergedStringsVisitor mergedVisitor(visitor);
      First->Process(mergedVisitor);
      Second->Process(mergedVisitor);
      mergedVisitor.ProcessRestStrings();
    }
  private:
    const Accessor::Ptr First;
    const Accessor::Ptr Second;
  };

  class StringConvertor : public StringMap
                        , public Visitor
  {
  public:
    virtual void SetIntValue(const NameType& name, IntType val)
    {
      insert(value_type(name, Helper(val)));
    }

    virtual void SetStringValue(const NameType& name, const StringType& val)
    {
      insert(value_type(name, Helper(val)));
    }

    virtual void SetDataValue(const NameType& name, const DataType& val)
    {
      insert(value_type(name, Helper(val)));
    }

  private:
    AsStringVisitor Helper;
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
        *it = highNibble * 16 | lowNibble;
      }
      return ValueType(res);
    }
    else if (IsInteger(val))
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
      return ValueType(negate ? -res : res);
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

  void DifferMaps(const Map& newOne, const Map& oldOne, Map& updates)
  {
    Map result;
    std::set_difference(newOne.begin(), newOne.end(),
      oldOne.begin(), oldOne.end(), std::inserter(result, result.end()),
      CompareParameter);
    updates.swap(result);
  }

  void MergeMaps(const Map& oldOne, const Map& newOne, Map& merged, bool replaceExisting)
  {
    Parameters::Map result;
    if (replaceExisting)
    {
      std::set_union(newOne.begin(), newOne.end(),
                     oldOne.begin(), oldOne.end(),
                     std::inserter(result, result.end()),
                     CompareParameter);
    }
    else
    {
      std::set_union(oldOne.begin(), oldOne.end(),
                     newOne.begin(), newOne.end(),
                     std::inserter(result, result.end()), CompareParameter);
    }
    merged.swap(result);
  }

  Accessor::Ptr CreateMergedAccessor(Accessor::Ptr first, Accessor::Ptr second)
  {
    return boost::make_shared<MergedAccessor>(first, second);
  }

  Container::Ptr Container::Create()
  {
    return boost::make_shared<ContainerImpl>();
  }

  void Convert(const Accessor& ac, StringMap& strings)
  {
    StringConvertor cnv;
    ac.Process(cnv);
    cnv.swap(strings);
  }
}
