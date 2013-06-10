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

//library includes
#include <strings/map.h>
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

  //! @brief Optional string quiotes
  const String::value_type STRING_QUOTE = '\'';
  //! @brief Mandatory data prefix
  const String::value_type DATA_PREFIX = '#';
  //@}

  class NameType
  {
    //! @brief Delimiter between namespaces in parameters' names
    static const char NAMESPACE_DELIMITER = '.';
  public:
    NameType()
    {
    }

    /*explicit*/NameType(const std::string& path)
      : Path(path)
    {
    }

    bool operator < (const NameType& rh) const
    {
      return Path < rh.Path;
    }

    bool operator == (const NameType& rh) const
    {
      return Path == rh.Path;
    }

    bool operator != (const NameType& rh) const
    {
      return Path != rh.Path;
    }

    bool IsEmpty() const
    {
      return Path.empty();
    }

    bool IsPath() const
    {
      return Path.npos != Path.find_first_of(NAMESPACE_DELIMITER);
    }

    bool IsSubpathOf(const NameType& sup) const
    {
      return !(*this - sup).IsEmpty();
    }

    NameType operator + (const NameType& rh) const
    {
      if (Path.empty())
      {
        return rh.Path.empty()
          ? NameType()
          : rh;
      }
      else
      {
        return rh.Path.empty()
          ? *this
          : NameType(Path + NAMESPACE_DELIMITER + rh.Path);
      }
    }

    NameType operator - (const NameType& sup) const
    {
      const std::string::size_type supSize = sup.Path.size();
      return supSize && Path.size() > supSize
          && 0 == Path.compare(0, supSize, sup.Path)
          && Path[supSize] == NAMESPACE_DELIMITER
        ? NameType(Path.substr(supSize + 1))
        : NameType();
    }

    NameType operator + (const std::string& rh) const
    {
      return operator + (NameType(rh));
    }

    NameType& operator = (const NameType& rh)
    {
      Path = rh.Path;
      return *this;
    }

    std::string FullPath() const
    {
      return Path;
    }

    std::string Name() const
    {
      const std::string::size_type lastDelim = Path.find_last_of(NAMESPACE_DELIMITER);
      return lastDelim != Path.npos
        ? Path.substr(lastDelim + 1)
        : Path;
    }
  private:
    std::string Path;
  };

  //! @brief Converting parameter value to string
  String ConvertToString(const IntType& val);
  String ConvertToString(const StringType& val);
  String ConvertToString(const DataType& val);

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
    virtual void SetValue(const NameType& name, IntType val) = 0;
    //! Add/modify string parameter
    virtual void SetValue(const NameType& name, const StringType& val) = 0;
    //! Add/modify data parameter
    virtual void SetValue(const NameType& name, const DataType& val) = 0;
  };

  void ParseStringMap(const Strings::Map& map, Visitor& visitor);

  class Modifier : public Visitor
  {
  public:
    //! Pointer type
    typedef boost::shared_ptr<Modifier> Ptr;

    //! Remove parameter
    virtual void RemoveValue(const NameType& name) = 0;
  };

  //! @brief Interface to give access to properties and parameters
  //! @invariant If value is not changed, parameter is not affected
  class Accessor
  {
  public:
    //! Pointer type
    typedef boost::shared_ptr<const Accessor> Ptr;

    virtual ~Accessor() {}

    virtual uint_t Version() const = 0;

    //! Accessing integer parameters
    virtual bool FindValue(const NameType& name, IntType& val) const = 0;
    //! Accessing string parameters
    virtual bool FindValue(const NameType& name, StringType& val) const = 0;
    //! Accessing data parameters
    virtual bool FindValue(const NameType& name, DataType& val) const = 0;

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
  void Convert(const Accessor& ac, Strings::Map& strings);


  class PropertyChangedCallback
  {
  public:
    virtual ~PropertyChangedCallback() {}

    virtual void OnPropertyChanged(const Parameters::NameType& name) const = 0;
  };

  Container::Ptr CreatePropertyTrackedContainer(Container::Ptr delegate, const PropertyChangedCallback& callback);


  template<class T>
  void CopyExistingValue(const Accessor& src, Visitor& dst, const NameType& name)
  {
    T val = T();
    if (src.FindValue(name, val))
    {
      dst.SetValue(name, val);
    }
  }
}

#endif //__PARAMETERS_TYPES_H_DEFINED__
