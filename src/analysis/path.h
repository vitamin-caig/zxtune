/**
*
* @file     analysis/path.h
* @brief    Interface for analyzed data path
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef ANALYSIS_PATH_H_DEFINED
#define ANALYSIS_PATH_H_DEFINED

//common includes
#include <iterator.h>
#include <types.h>
//boost includes
#include <boost/shared_ptr.hpp>

namespace Analysis
{
  //! @brief Simple analyzed subdata identifier
  class Path
  {
  public:
    typedef boost::shared_ptr<const Path> Ptr;
    typedef ObjectIterator<String> Iterator;

    virtual ~Path() {}

    //! @brief Check if path is empty
    //! @return true if no significant elements
    virtual bool Empty() const = 0;
    //! @brief Serialization
    //! @return String presentation of path
    virtual String AsString() const = 0;
    //! @brief Path elements iterating
    //! @return Iterator by strings
    //! @invariant Always not-null result
    virtual Iterator::Ptr GetIterator() const = 0;
    //! @brief Append path to current
    //! @param element Subpath to be appended. May be complex
    //! @return New path object
    //! @invariant Result is always not null
    //! @invariant Current object is not changed
    virtual Ptr Append(const String& element) const = 0;
    //! @brief Extract subpath from current
    //! @param startPath Part of current path starting from beginning
    //! @return Not-null object is AsString() is started of startPath (even in full match case), null elsewhere
    virtual Ptr Extract(const String& startPath) const = 0;
  };

  Path::Ptr ParsePath(const String& str, Char separator);
}

#endif //ANALYSIS_PATH_H_DEFINED
