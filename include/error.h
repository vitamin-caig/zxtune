/**
*
* @file     error.h
* @brief    %Error subsystem definitions
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef ERROR_H_DEFINED
#define ERROR_H_DEFINED

//common includes
#include <types.h>
//boost includes
#include <boost/shared_ptr.hpp>

//! @class Error
//! @brief Error subsystem core class. Can be used as a return value or throw object
class Error
{
  // internal types
  struct Meta;
  typedef boost::shared_ptr<Meta> MetaPtr;

  void TrueFunc() const
  {
  }
public:
  //! @brief Datatype for source text line identification\n
  //! LineTag = FILE_TAG + __LINE__\n
  //! assume __LINE__ < 65536
  typedef uint32_t LineTag;

#ifndef NDEBUG
  //! @struct Location
  //! @brief %Location type for debug builds
  struct Location
  {
    //! Default constructor
    Location() : Tag(0), File(0), Function(0), Line(0)
    {
    }
    //! Full parameters list constructor
    Location(LineTag tag, const char* file, const char* function, uint_t line)
      : Tag(tag), File(file), Function(function), Line(line)
    {
    }
    //! Tag (same as in release version)
    LineTag Tag;
    //! Filename where exception was created
    const char* File;
    //! Source function
    const char* Function;
    //! Source line
    uint_t Line;

    bool operator == (const Location& rh) const
    {
      return Tag == rh.Tag;
    }
  };
  //! @typedef const Location& LocationRef
  //! @brief Reference type for debug location
  typedef const Location& LocationRef;
#else
  //! @typedef LineTag Location
  //! @brief %Location type for release builds
  typedef LineTag Location;
  //! @typedef Location LocationRef
  //! @brief Reference type for release location
  typedef Location LocationRef;
#endif
  //@{
  //! @name Error initializers
  Error();//success

  //! @code
  //! return Error(THIS_LINE, ERROR_TEXT);
  //! @endcode
  Error(LocationRef loc, const String& text);

  Error(const Error& rh) : ErrorMeta(rh.ErrorMeta)
  {
  }

  Error& operator = (const Error& rh)
  {
    ErrorMeta = rh.ErrorMeta;
    return *this;
  }
  //@}

  //@{
  //! @name Hierarchy-related functions

  //! @brief Adding suberror
  //! @param e Reference to other object. Empty objects are ignored
  //! @return Modified current object
  //! @note Error uses shared references scheme, so it's safe to use parameter again
  Error& AddSuberror(const Error& e);
  Error GetSuberror() const;
  //@}

  //@{
  //! @name Content accessors

  //! @brief Text accessor
  String GetText() const;
  //! @brief Location accessor
  Location GetLocation() const;

  typedef void (Error::*BoolType)() const;

  //! @brief Check if error not empty
  operator BoolType () const;

  //! @brief Checking if error is empty
  bool operator ! () const;
  //@}

  //@{
  //! @name Serialization-related functions

  //! @brief Convert error and all suberrors to single string using AttributesToString function
  String ToString() const;
  //@}
private:
  Error(MetaPtr ptr) : ErrorMeta(ptr)
  {
  }
private:
  MetaPtr ErrorMeta;
};

//! @brief Tool function used for check result of function and throw error in case of unsuccess
inline void ThrowIfError(const Error& e)
{
  if (e)
  {
    throw e;
  }
}

//! @def THIS_LINE
//! @brief Macro is used for bind created error with current source line

#define MKTAG1(a) 0x ## a
#define MKTAG(a) MKTAG1(a)
#define MAKETAG (MKTAG(FILE_TAG) + __LINE__)
#ifndef NDEBUG
  #define THIS_LINE (Error::Location(MAKETAG, __FILE__, __FUNCTION__, __LINE__))
#else
  #define THIS_LINE (Error::Location(MAKETAG))
#endif

#endif //ERROR_H_DEFINED
