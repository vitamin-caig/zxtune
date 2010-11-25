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

  //! @brief Interface to modify properties and parameters
  class Modifier
  {
  public:
    //! Pointer type
    typedef boost::shared_ptr<Modifier> Ptr;

    virtual ~Modifier() {}

    //! Modifying integer parameters
    virtual void SetIntValue(const NameType& name, IntType val) = 0;
    //! Modifying string parameters
    virtual void SetStringValue(const NameType& name, const StringType& val) = 0;
    //! Modifying data parameters
    virtual void SetDataValue(const NameType& name, const DataType& val) = 0;
  };

  void ParseStringMap(const StringMap& map, Modifier& modifier);

  //! @brief Use modifier interface as a visitor
  typedef Modifier Visitor;

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
}

#endif //__PARAMETERS_TYPES_H_DEFINED__
