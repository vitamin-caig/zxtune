/*
Abstract:
  Error subsystem implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include <error_tools.h>
#include <string_helpers.h>

#include <cctype>
#include <iomanip>

#include <text/tools.h>

// implementation of error's core used to keep data
struct Error::Meta
{
  Meta() : Location(), Code(), Text()
  {
  }
  
  Meta(LocationRef loc, CodeType code, const String& txt)
    : Location(loc), Code(code), Text(txt)
  {
    assert(Code);
  }
  Meta(LocationRef loc, CodeType code)
    : Location(loc), Code(code)
  {
    assert(Code);
  }

  Error::Location Location;
  Error::CodeType Code;
  String Text;
  MetaPtr Suberror;

  static Meta* Success()
  {
    static Meta Success;
    return &Success;
  }

  // static destructor to release error in allocate place (workaround against multiple runtimes)
  static void Delete(Meta* obj)
  {
    if (obj != Success())
    {
      delete obj;
    }
  }
};

Error::Error() : ErrorMeta(Meta::Success(), Meta::Delete)
{
}

Error::Error(LocationRef loc, CodeType code)
  : ErrorMeta(new Meta(loc, code), Meta::Delete)
{
}

Error::Error(LocationRef loc, CodeType code, const String& txt)
  : ErrorMeta(new Meta(loc, code, txt), Meta::Delete)
{
}

Error& Error::AddSuberror(const Error& e)
{
  //do not add/add to 'success' error
  if (e.GetCode() && GetCode())
  {
    MetaPtr ptr = ErrorMeta;
    while (ptr->Suberror)
    {
      ptr = ptr->Suberror;
    }
    ptr->Suberror = e.ErrorMeta;
  }
  return *this;
}

Error Error::FindSuberror(CodeType code) const
{
  MetaPtr ptr = ErrorMeta;
  while (ptr && ptr->Code != code)
  {
    ptr = ptr->Suberror;
  }
  return ptr ? Error(ptr) : Error();
}

void Error::WalkSuberrors(const boost::function<void(uint_t, LocationRef, CodeType, const String&)>& callback) const
{
  MetaPtr ptr = ErrorMeta;
  for (uint_t level = 0; ptr; ++level, ptr = ptr->Suberror)
  {
    const Meta& Meta(*ptr);
    callback(level, Meta.Location, Meta.Code, Meta.Text);
  }
}

const String& Error::GetText() const
{
  return ErrorMeta->Text;
}

Error::CodeType Error::GetCode() const
{
  return ErrorMeta->Code;
}

Error::operator Error::CodeType () const
{
  return ErrorMeta->Code;
}

bool Error::operator ! () const
{
  return ErrorMeta->Code == 0;
}

String Error::CodeToString(CodeType code)
{
  const std::size_t codeBytes(sizeof(code) - 3);
  const CodeType syms = code >> (8 * codeBytes);
  const Char p1 = syms & 0xff;
  const Char p2 = (syms >> 8) & 0xff;
  const Char p3 = (syms >> 16) & 0xff;
  OutStringStream str;
  if (std::isalnum(p1) && std::isalnum(p2) && std::isalnum(p3))
  {
    str << p1 << p2 << p3 << Char('#') << std::setw(2 * codeBytes) << std::setfill(Char('0')) << std::hex << (code & ((uint_t(1) << 8 * codeBytes) - 1));
  }
  else
  {
    str << Char('0') << Char('x') << std::setw(2 * sizeof(code)) << std::setfill(Char('0')) << std::hex << code;
  }
  return str.str();
}

String Error::AttributesToString(LocationRef loc, CodeType code, const String& text)
{
  return (Formatter(TEXT_ERROR_DEFAULT_FORMAT) % text % CodeToString(code) % LocationToString(loc)).str();
}

String Error::LocationToString(Error::LocationRef loc)
{
#ifdef NDEBUG
  return (Formatter(TEXT_ERROR_LOCATION_FORMAT) % loc).str();
#else
  return (Formatter(TEXT_ERROR_LOCATION_FORMAT_DEBUG) % loc.Tag % loc.File % loc.Line % loc.Function).str();
#endif
}
