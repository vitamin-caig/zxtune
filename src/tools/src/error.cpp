/**
 *
 * @file
 *
 * @brief  Error object implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// common includes
#include <error_tools.h>
// std includes
#include <utility>

namespace
{
  String AttributesToString(Error::LocationRef loc, const String& text) throw()
  {
    try
    {
#ifdef NDEBUG
      constexpr const Char FORMAT[] =
          "{0}\n"
          "@{1:08x}\n"
          "--------\n";
      return Strings::Format(FORMAT, text, loc);
#else
      constexpr const Char FORMAT[] =
          "{0}\n"
          "@{1:08x} ({2}:{3}, {4})\n"
          "--------\n";
      return Strings::Format(FORMAT, text, loc.Tag, loc.File, loc.Line, loc.Function);
#endif
    }
    catch (const std::exception& e)
    {
      return e.what();
    }
  }
}  // namespace

// implementation of error's core used to keep data
struct Error::Meta
{
  Meta(LocationRef loc, String txt)
    : Location(std::move(loc))
    , Text(std::move(txt))
  {}

  // source error location
  Error::Location Location;
  // error text message
  String Text;
  // suberror
  MetaPtr Suberror;
};

Error::Error(LocationRef loc, const String& txt)
  : ErrorMeta(std::make_shared<Meta>(loc, txt))
{}

Error& Error::AddSuberror(const Error& e)
{
  // do not add/add to 'success' error
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
  return ErrorMeta ? &Error::TrueFunc : nullptr;
}

bool Error::operator!() const
{
  return !ErrorMeta;
}

String Error::ToString() const throw()
{
  String details;
  for (MetaPtr meta = ErrorMeta; meta; meta = meta->Suberror)
  {
    details += AttributesToString(meta->Location, meta->Text);
  }
  return details;
}
