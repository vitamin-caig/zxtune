/**
*
* @file     parameters.h
* @brief    Parameters-related types definitions
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef __PARAMETERS_TYPES_H_DEFINED__
#define __PARAMETERS_TYPES_H_DEFINED__

//common includes
#include <string_helpers.h>
//boost includes
#include <boost/shared_ptr.hpp>

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

  //! @brief Delimiter between namespaces in parameters' names
  const NameType::value_type NAMESPACE_DELIMITER = '.';
  //! @brief Optional string quiotes
  const String::value_type STRING_QUOTE = '\'';
  //! @brief Mandatory data prefix
  const String::value_type DATA_PREFIX = '#';
  //@}

  //! @brief Converting parameter value to string
  String ConvertToString(IntType val);
  String ConvertToString(StringType val);
  String ConvertToString(DataType val);

  //! @brief Converting parameter value from string
  bool ConvertFromString(const String& str, IntType& res);
  bool ConvertFromString(const String& str, StringType& res);
  bool ConvertFromString(const String& str, DataType& res);

  //! @brief Interface to add/modify properties and parameters
  class Visitor
  {
  public:
    //! Pointer type
    typedef boost::shared_ptr<Visitor> Ptr;

    virtual ~Visitor() {}

    //! Add/modify integer parameter
    virtual void SetIntValue(const NameType& name, IntType val) = 0;
    //! Add/modify string parameter
    virtual void SetStringValue(const NameType& name, const StringType& val) = 0;
    //! Add/modify data parameter
    virtual void SetDataValue(const NameType& name, const DataType& val) = 0;
  };

  void ParseStringMap(const StringMap& map, Visitor& visitor);

  class Modifier : public Visitor
  {
  public:
    //! Pointer type
    typedef boost::shared_ptr<Modifier> Ptr;

    //! Remove integer parameter
    virtual void RemoveIntValue(const NameType& name) = 0;
    //! Remove string parameter
    virtual void RemoveStringValue(const NameType& name) = 0;
    //! Remove data parameter
    virtual void RemoveDataValue(const NameType& name) = 0;
  };

  //! @brief Interface to give access to properties and parameters
  //! @invariant If value is not changed, parameter is not affected
  class Accessor
  {
  public:
    //! Pointer type
    typedef boost::shared_ptr<const Accessor> Ptr;

    virtual ~Accessor() {}

    //! Accessing integer parameters
    virtual bool FindIntValue(const NameType& name, IntType& val) const = 0;
    //! Accessing string parameters
    virtual bool FindStringValue(const NameType& name, StringType& val) const = 0;
    //! Accessing data parameters
    virtual bool FindDataValue(const NameType& name, DataType& val) const = 0;

    //! Valk along the stored values
    virtual void Process(Visitor& visitor) const = 0;
  };

  // All accessed properties are prioritized by the first one
  Accessor::Ptr CreateMergedAccessor(Accessor::Ptr first, Accessor::Ptr second);
  Accessor::Ptr CreateMergedAccessor(Accessor::Ptr first, Accessor::Ptr second, Accessor::Ptr third);

  //! @brief Service type to simply properties keep and give access
  //! @invariant Only last value is kept for multiple assignment
  class Container : public Accessor
                  , public Modifier
  {
  public:
    //! Pointer type
    typedef boost::shared_ptr<Container> Ptr;

    static Ptr Create();
  };

  //other functions
  void Convert(const Accessor& ac, StringMap& strings);


  class PropertyChangedCallback
  {
  public:
    virtual ~PropertyChangedCallback() {}

    virtual void OnPropertyChanged(const Parameters::NameType& name) const = 0;
  };

  Container::Ptr CreatePropertyTrackedContainer(Container::Ptr delegate, const PropertyChangedCallback& callback);
}

#endif //__PARAMETERS_TYPES_H_DEFINED__
