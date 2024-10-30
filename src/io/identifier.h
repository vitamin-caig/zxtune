/**
 *
 * @file
 *
 * @brief  IO-specific data identifier interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <string_type.h>
#include <string_view.h>
// std includes
#include <memory>

namespace IO
{
  /*
    Common uri identifier is not suitable due to possible format violations
    (e.g. file path may contain '#' symbols and subpath may contain '/' symbols which are not allowed in uri's fragment)
  */
  class Identifier
  {
  public:
    using Ptr = std::shared_ptr<const Identifier>;

    virtual ~Identifier() = default;

    //! @return Full identifier
    virtual String Full() const = 0;

    //! @return Mandatory scheme
    virtual String Scheme() const = 0;
    //! @return Mandatory path
    virtual String Path() const = 0;
    //! @return Optional file name
    virtual String Filename() const = 0;
    //! @return Optional file extension
    virtual String Extension() const = 0;
    //! @return Optional nested data subpath
    virtual String Subpath() const = 0;

    //! Builders
    virtual Ptr WithSubpath(StringView subpath) const = 0;
  };
}  // namespace IO
