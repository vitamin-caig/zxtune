/**
*
* @file     parameters.h
* @brief    Parameters-related types definitions
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#ifndef __PARAMETERS_TYPES_H_DEFINED__
#define __PARAMETERS_TYPES_H_DEFINED__

//common includes
#include <string_helpers.h>
//std includes
#include <map>
//boost includes
#include <boost/variant/get.hpp>
#include <boost/variant/variant.hpp>

//! @brief Namespace is used to keep parameters-working related types and functions
namespace Parameters
{
  //@{
  //! @name Value types
  
  //! @brief Integer parameters type
  typedef int64_t IntType;
  //! @brief String parameters type
  typedef String StringType;
  //! @brief Data parameters type
  typedef Dump DataType;
  //@}
  
  //@{
  //! @name Other types
  
  //! @brief %Parameters name type
  typedef String NameType;
  //! @brief Complex variant value type
  typedef boost::variant<IntType, StringType, DataType> ValueType;
  //! @brief %Parameters map type
  typedef std::map<NameType, ValueType> Map;
  //@}
  
  //! @brief Delimiter between namespaces in parameters' names
  const NameType::value_type NAMESPACE_DELIMITER = '.';
  //! @brief Optional string quiotes
  const String::value_type STRING_QUOTE = '\'';
  //! @brief Mandatory data prefix
  const String::value_type DATA_PREFIX = '#';
  
  //@{
  //! @name Working with map

  //! @brief Searching for the parameter of specified type and name
  //! @tparam T Parameter type
  //! @param params Input map
  //! @param name Parameter name
  //! @return 0 if not found
  template<class T>
  inline const T* FindByName(const Map& params, const NameType& name)
  {
    const Map::const_iterator it(params.find(name));
    return it != params.end() ? boost::get<T>(&(it->second)) : 0;
  }
  
  //! @brief Searching for the parameter of specified type and name
  //! @param params Input map
  //! @param name Parameter name
  //! @param result return value
  //! @return true if parameter is found and copied to result
  template<class T>
  inline bool FindByName(const Map& params, const NameType& name, T& result)
  {
    const T* const val = FindByName<T>(params, name);
    return val ? (result = *val, true) : false;
  }

  //! @brief Converting parameter value to string
  String ConvertToString(const ValueType& val);
  //! @brief Converting string to parameter value
  ValueType ConvertFromString(const String& val);
  //! @brief Converting Map to #StringMap
  void ConvertMap(const Map& input, StringMap& output);
  //! @brief Converting #StringMap to Map
  void ConvertMap(const StringMap& input, Map& output);
  //! @brief Calculating difference between to maps
  void DifferMaps(const Map& newOne, const Map& oldOne, Map& updates);
  //! @brief Merging two maps
  void MergeMaps(const Map& oldOne, const Map& newOne, Map& merged, bool replaceExisting);

  //! @brief Simple helper to work with parameters
  class Helper
  {
  public:
    explicit Helper(const Map& content)
      : Content(content)
    {
    }

    template<class T>
    const T* FindValue(const NameType& name) const
    {
      const Map::const_iterator it = Content.find(name);
      return it != Content.end() ? boost::get<T>(&(it->second)) : 0;
    }

    template<class T>
    bool FindValue(const NameType& name, T& result) const
    {
      const T* const val = FindByName<T>(name);
      return val ? (result = *val, true) : false;
    }

    template<class T>
    T GetValue(const NameType& name, const T& defaultValue) const
    {
      const T* const val = FindValue<T>(name);
      return val ? *val : defaultValue;
    }

    void ToStringMap(StringMap& result) const
    {
      ConvertMap(Content, result);
    }
  private:
    const Map& Content;
  };
}

#endif //__PARAMETERS_TYPES_H_DEFINED__
