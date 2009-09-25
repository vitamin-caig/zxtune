/*
Abstract:
  Error subsystem definitions

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef __ERROR_H_DEFINED__
#define __ERROR_H_DEFINED__

#include <types.h>

#include <boost/function.hpp>
#include <boost/smart_ptr.hpp>

class Error
{
  struct Meta;
  typedef boost::shared_ptr<struct Meta> MetaPtr;
public:
  //Datatype for source text line identification
  //LineTag=FILE_TAG+__LINE__
  //assume __LINE__ < 65536
  typedef uint32_t LineTag;
  //type for code
  typedef uint32_t CodeType;

  //helper template used for generate per-module base error code
  template<uint8_t p1, uint8_t p2>
  struct ModuleCode
  {
    static const CodeType Value = (CodeType(p1) | (CodeType(p2) << 8)) << (8 * (sizeof(CodeType) - 2));
  };

#ifndef NDEBUG
  struct Location
  {
    Location() : Tag(0), File(0), Function(0), Line(0)
    {
    }
    Location(LineTag tag, const char* file, const char* function, std::size_t line)
      : Tag(tag), File(file), Function(function), Line(line)
    {
    }
    LineTag Tag;
    const char* File;
    const char* Function;
    std::size_t Line;
    
    bool operator == (const Location& rh) const
    {
      return Tag == rh.Tag;
    }
  };
  typedef const Location& LocationRef;
#else
  typedef LineTag Location;
  typedef Location LocationRef;
#endif

  //ctors
  Error();//success
  Error(LocationRef loc, CodeType code, const String& text);
  ~Error()
  {
  }
  
  //copy ctors and assigner
  Error(const Error& rh) : ErrorMeta(rh.ErrorMeta)
  {
  }
  Error& operator = (const Error& rh)
  {
    ErrorMeta = rh.ErrorMeta;
    return *this;
  }
  
  //hierarchy members
  Error& AddSuberror(const Error& e);
  Error FindSuberror(CodeType code) const;
  //walk through all nested suberrors
  //callback(level, detail, code, text)
  void WalkSuberrors(const boost::function<void(unsigned, LocationRef, CodeType, const String&)>& callback) const;
  
  //getters
  const String& GetText() const;
  CodeType GetCode() const;
  
  //checkers
  operator CodeType () const;
  bool operator ! () const;
  
  //serialize
  static String LocationToString(LocationRef loc);
private:
  Error(MetaPtr ptr) : ErrorMeta(ptr)
  {
  }
private:
  MetaPtr ErrorMeta;
};

template<class P1>
inline Error MakeFormattedError(Error::LocationRef loc, Error::CodeType code, const String& fmt,
  const P1& p1)
{
  return Error(loc, code, (Formatter(fmt) % p1).str());
}

template<class P1, class P2>
inline Error MakeFormattedError(Error::LocationRef loc, Error::CodeType code, const String& fmt,
  const P1& p1, const P2& p2)
{
  return Error(loc, code, (Formatter(fmt) % p1 % p2).str());
}

#define MKTAG1(a) 0x ## a
#define MKTAG(a) MKTAG1(a)
#define MAKETAG (MKTAG(FILE_TAG) + __LINE__)
#ifndef NDEBUG
  #define THIS_LINE (Error::Location(MAKETAG, __FILE__, __FUNCTION__, __LINE__))
#else
  #define THIS_LINE (Error::Location(MAKETAG))
#endif

#endif //__ERROR_H_DEFINED__
