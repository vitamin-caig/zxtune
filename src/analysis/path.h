/**
 *
 * @file
 *
 * @brief Analyzed data path interface and factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <string_type.h>
#include <string_view.h>
// std includes
#include <span>

namespace Analysis
{
  //! @brief Simple analyzed subdata identifier
  class Path
  {
  public:
    using Ptr = std::shared_ptr<const Path>;

    virtual ~Path() = default;

    //! @brief Check if path is empty
    //! @return true if no significant elements
    virtual bool Empty() const = 0;
    //! @brief Serialization
    //! @return String presentation of path
    virtual String AsString() const = 0;
    //! @brief Path elements
    virtual std::span<const String> Elements() const = 0;
    //! @brief Append path to current
    //! @param element Subpath to be appended. May be complex
    //! @return New path object
    //! @invariant Result is always not null
    //! @invariant Current object is not changed
    virtual Ptr Append(StringView element) const = 0;
    //! @brief Extract subpath from current
    //! @param startPath Part of current path starting from beginning
    //! @return Not-null object is AsString() is started of startPath (even in full match case), null elsewhere
    virtual Ptr Extract(StringView startPath) const = 0;
    //! @brief Return all the path elements but the last one
    //! @return Ptr() if this->Empty(), non-null (but may be empty) object instead
    virtual Ptr GetParent() const = 0;
  };

  Path::Ptr ParsePath(StringView str, char separator);
}  // namespace Analysis
