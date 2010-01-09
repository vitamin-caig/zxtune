/*
Abstract:
  Parameters-related types definitions

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef __PARAMETERS_TYPES_H_DEFINED__
#define __PARAMETERS_TYPES_H_DEFINED__

#include <string_helpers.h>
#include <types.h>

#include <map>

#include <boost/variant/get.hpp>
#include <boost/variant/variant.hpp>

namespace Parameters
{
  typedef int64_t IntType;
  typedef String StringType;
  typedef Dump DataType;
  
  typedef String NameType;
  typedef boost::variant<IntType, StringType, DataType> ValueType;
  typedef std::map<NameType, ValueType> Map;
  
  const NameType::value_type NAMESPACE_DELIMITER = '.';
  
  //working with parameters map
  template<class T>
  inline const T* FindByName(const Map& params, const NameType& name)
  {
    const Map::const_iterator it(params.find(name));
    return it != params.end() ? boost::get<T>(&(it->second)) : 0;
  }
  
  template<class T>
  inline bool FindByName(const Map& params, const NameType& name, T& result)
  {
    const T* const val = FindByName<T>(params, name);
    return val ? (result = *val, true) : false;
  }
  
  String ConvertToString(const ValueType& val);
  ValueType ConvertFromString(const String& val);
  void ConvertMap(const Map& input, StringMap& output);
  void ConvertMap(const StringMap& input, Map& output);
}

#endif //__PARAMETERS_TYPES_H_DEFINED__
