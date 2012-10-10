/*
Abstract:
  Error subsystem implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//common includes
#include <error_tools.h>
//library includes
#include <l10n/api.h>
//std includes
//#include <cctype>
//boost includes
#include <boost/make_shared.hpp>
//text includes
#include <tools/text/tools.h>

namespace
{
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("tools");

  String LocationToString(Error::LocationRef loc)
  {
  #ifdef NDEBUG
    return Strings::Format(Text::ERROR_LOCATION_FORMAT, loc);
  #else
    return Strings::Format(Text::ERROR_LOCATION_FORMAT_DEBUG, loc.Tag, loc.File, loc.Line, loc.Function);
  #endif
  }

  String AttributesToString(Error::LocationRef loc, const String& text)
  {
    return Strings::Format(translate("%1%\n\nAt: %2%\n--------\n"), text, LocationToString(loc));
  }
}

// implementation of error's core used to keep data
struct Error::Meta
{
  Meta() : Location(), Text()
  {
  }
  
  Meta(LocationRef loc, const String& txt)
    : Location(loc), Text(txt)
  {
  }

  explicit Meta(LocationRef loc)
    : Location(loc)
  {
  }

  // source error location
  Error::Location Location;
  // error text message
  String Text;
  // suberror
  MetaPtr Suberror;
};

Error::Error() : ErrorMeta()
{
}

Error::Error(LocationRef loc, const String& txt)
  : ErrorMeta(boost::make_shared<Meta>(loc, txt))
{
}

Error& Error::AddSuberror(const Error& e)
{
  //do not add/add to 'success' error
  if (e && *this)
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

Error Error::GetSuberror() const
{
  return ErrorMeta && ErrorMeta->Suberror ? Error(ErrorMeta->Suberror) : Error();
}

String Error::GetText() const
{
  return ErrorMeta ? ErrorMeta->Text : String();
}

Error::Location Error::GetLocation() const
{
  return ErrorMeta ? ErrorMeta->Location : Error::Location();
}

Error::operator Error::BoolType() const
{
  return ErrorMeta ? &Error::TrueFunc : 0;
}

bool Error::operator ! () const
{
  return !ErrorMeta;
}

String Error::ToString() const
{
  String details;
  for (MetaPtr meta = ErrorMeta; meta; meta = meta->Suberror)
  {
    details += AttributesToString(meta->Location, meta->Text);
  }
  return details;
}
