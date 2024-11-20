/**
 *
 * @file
 *
 * @brief  %Error subsystem definitions
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "source_location.h"  // IWYU pragma: export
#include "string_type.h"
#include "types.h"

#include <memory>
#include <utility>

//! @class Error
//! @brief Error subsystem core class. Can be used as a return value or throw object
class Error
{
public:
  using Location = uint_t;
  using LocationRef = Location;  // TODO: remove

  //@{
  //! @name Error initializers
  Error() = default;  // success

  //! @code
  //! return Error(THIS_LINE, ERROR_TEXT);
  //! @endcode
  Error(Location loc, String text) noexcept
    : ErrorMeta(std::make_shared<Meta>(loc, std::move(text)))
  {}

  Error(const Error&) = default;
  Error(Error&& rh) noexcept = default;

  Error& operator=(const Error&) = default;
  Error& operator=(Error&& rh) noexcept = default;
  //@}

  //@{
  //! @name Hierarchy-related functions

  //! @brief Adding suberror
  //! @param e Reference to other object. Empty objects are ignored
  //! @return Lightweight copy of current possibly modified object
  //! @note Error uses shared references scheme, so it's safe to use parameter again
  Error AddSuberror(const Error& e) noexcept
  {
    // do not add/add to 'success' error
    if (e && *this)
    {
      auto ptr = ErrorMeta;
      while (ptr->Suberror)
      {
        ptr = ptr->Suberror;
      }
      ptr->Suberror = e.ErrorMeta;
    }
    return Error(ErrorMeta);
  }

  Error GetSuberror() const noexcept
  {
    return ErrorMeta && ErrorMeta->Suberror ? Error(ErrorMeta->Suberror) : Error();
  }
  //@}

  //@{
  //! @name Content accessors

  //! @brief Text accessor
  const String& GetText() const noexcept
  {
    static const String EMPTY;
    return ErrorMeta ? ErrorMeta->Text : EMPTY;
  }

  //! @brief Location accessor
  Location GetLocation() const noexcept
  {
    return ErrorMeta ? ErrorMeta->Source : Location();
  }

  //! @brief Check if error not empty
  explicit operator bool() const
  {
    return !!ErrorMeta;
  }

  //! @brief Checking if error is empty
  bool operator!() const
  {
    return !ErrorMeta;
  }
  //@}

  //@{
  //! @name Serialization-related functions

  //! @brief Convert error and all suberrors to single string using AttributesToString function
  String ToString() const noexcept;
  //@}
private:
  // internal types
  struct Meta
  {
    using Ptr = std::shared_ptr<Meta>;

    Meta(Location src, String txt)
      : Source(src)
      , Text(std::move(txt))
    {}

    // source error location
    const Location Source;
    // error text message
    const String Text;
    // suberror
    Ptr Suberror;
  };

  explicit Error(Meta::Ptr ptr)
    : ErrorMeta(std::move(ptr))
  {}

private:
  Meta::Ptr ErrorMeta;
};

//! @brief Tool function used for check result of function and throw error in case of unsuccess
inline void ThrowIfError(const Error& e) noexcept(false)
{
  if (e)
  {
    throw e;
  }
}

//! @def THIS_LINE
//! @brief Macro is used for bind created error with current source line

#define THIS_LINE (Error::Location(ThisLine().Tag()))
