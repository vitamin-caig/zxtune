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

#include <types.h>

#include <boost/variant.hpp>

namespace Parameters
{
  typedef int64_t IntType;
  typedef String StringType;
  typedef Dump DataType;
  
  typedef boost::variant<IntType, StringType, DataType> VariantType;
  typedef std::map<String, VariantType> Map;
  
  //working with parameters map
  template<class T>
  inline const T* FindByName(const Map& params, const String& name)
  {
    const Map::const_iterator it(params.find(name));
    return it != params.end() ? boost::get<T>(&(it->second)) : 0;
  }
  
  template<class T>
  inline bool FindByName(const Map& params, const String& name, T& result)
  {
    const T* const val = FindByName<T>(params, name);
    return val ? (result = *val, true) : false;
  }
}

#endif //__PARAMETERS_TYPES_H_DEFINED__
