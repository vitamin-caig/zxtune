/**
*
* @file     error.h
* @brief    %Error subsystem definitions
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#ifndef __ERROR_H_DEFINED__
#define __ERROR_H_DEFINED__

#include <string_type.h>
#include <types.h>

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

//! @class Error
//! @brief Error subsystem core class. Can be used as a return value or throw object
class Error
{
  // internal types
  struct Meta;
  typedef boost::shared_ptr<struct Meta> MetaPtr;
public:
  //! @typedef uint32_t LineTag
  //! @brief Datatype for source text line identification\n
  //! LineTag = FILE_TAG + __LINE__\n
  //! assume __LINE__ < 65536
  typedef uint32_t LineTag;
  //! @typedef uint32_t CodeType
  //! @brief Type for code
  typedef uint32_t CodeType;

  //! @struct ModuleCode 
  //! @brief Template used for generate per-module base error code:
  //! @code 
  //! const Error::CodeType ThisModuleCode = Error::ModuleCode<'A', 'B', 'C'>::Value;
  //! @endcode
  template<uint8_t p1, uint8_t p2, uint8_t p3>
  struct ModuleCode
  {
    static const CodeType Value = (CodeType(p1) | (CodeType(p2) << 8) | (CodeType(p3) << 16)) << (8 * (sizeof(CodeType) - 3));
  };

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
    Location(LineTag tag, const char* file, const char* function, unsigned line)
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
    unsigned Line;
    
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
  Error(LocationRef loc, CodeType code);
  //! @code
  //! return Error(THIS_LINE, Error::ModuleCode<'A', 'B', 'C'>::Value, ERROR_TEXT);
  //! @endcode
  Error(LocationRef loc, CodeType code, const String& text);
  ~Error()
  {
  }
  
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
  //! @brief Searching for suberror in stack by code
  //! @param code %Error code
  //! @return Empty object in case of fail or new object with specified code
  Error FindSuberror(CodeType code) const;
  //! @brief Walking through all nested suberrors
  //! @param callback Callback which will be called on each level by stack starting from 0
  void WalkSuberrors(const boost::function<void(unsigned, LocationRef, CodeType, const String&)>& callback) const;
  //@}
  
  //@{
  //! @name Content accessors
  
  //! @brief Text accessor
  const String& GetText() const;
  //! @brief Code accessor
  CodeType GetCode() const;
  
  //! @brief Implicit casting to error code
  operator CodeType () const;
  //! @brief Checking if %Code != 0
  bool operator ! () const;
  //@}
  
  //@{
  //! @name Serialization-related functions
  
  //! @brief Converting location to string
  //! @note @ref Location::Tag (hex) format is used for release
  //! @note @ref Location::Tag (hex) (@ref Location::File : @ref Location::Line, @ref Location::Function) format is used for debug
  static String LocationToString(LocationRef loc);
  //! @brief Converting code to string
  //! @note ABC#HHHH if code is composed using ModuleCode<'A', 'B', 'C'>::Value + DDD
  //! @note 0xHHHHHHHH else
  static String CodeToString(CodeType code);
  //! @brief Converts all the attributes to single string using internal format
  //! @param loc %Error location (debug or release)
  //! @param code %Error code
  //! @param text %Error text
  //! @return
  //! @code
  //! ${text}
  //!
  //! Code: ${code}
  //! At: ${loc}
  //! ------
  //! @endcode
  static String AttributesToString(LocationRef loc, CodeType code, const String& text);
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

#endif //__ERROR_H_DEFINED__
