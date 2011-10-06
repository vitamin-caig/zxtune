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
  class Path
  {
  public:
    typedef boost::shared_ptr<const Path> Ptr;
    typedef ObjectIterator<String> Iterator;

    virtual ~Path() {}

    virtual bool Empty() const = 0;
    virtual String AsString() const = 0;
    virtual Iterator::Ptr GetIterator() const = 0;
    virtual Ptr Append(const String& element) const = 0;
    virtual Ptr Extract(const String& startPath) const = 0;
  };

  Path::Ptr ParsePath(const String& str);
}

#endif //ANALYSIS_PATH_H_DEFINED
