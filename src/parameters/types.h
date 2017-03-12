/**
*
* @file
*
* @brief  Parameters-related types definitions
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//common includes
#include <types.h>

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
    NameType() = default;

    /*explicit*/NameType(std::string path)
      : Path(std::move(path))
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

    NameType operator + (std::string rh) const
    {
      return operator + (NameType(std::move(rh)));
    }

    NameType& operator = (const NameType& rh) = default;

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
}
