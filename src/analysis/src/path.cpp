/*
Abstract:
  Analyzed data path implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//common includes
#include <string_helpers.h>
//library includes
#include <analysis/path.h>
//boost includes
#include <boost/make_shared.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>
//std includes
#include <cassert>

namespace
{
  const String DELIMITER(1, '/');

  StringArray SplitPath(const String& str)
  {
    StringArray parts;
    boost::algorithm::split(parts, str, boost::algorithm::is_any_of(DELIMITER), boost::algorithm::token_compress_on);
    const StringArray::iterator newEnd = std::remove_if(parts.begin(), parts.end(),
      std::mem_fun_ref(&String::empty));
    return StringArray(parts.begin(), newEnd);
  }

  String JoinPath(const StringArray& arr)
  {
    return boost::algorithm::join(arr, DELIMITER);
  }

  class ParsedPath : public Analysis::Path
  {
  public:
    explicit ParsedPath(const StringArray& components)
      : Components(components)
    {
    }

    template<class Iter>
    ParsedPath(Iter from, Iter to)
      : Components(from, to)
    {
    }

    virtual bool Empty() const
    {
      return Components.empty();
    }

    virtual String AsString() const
    {
      return JoinPath(Components);
    }

    virtual Iterator::Ptr GetIterator() const
    {
      return CreateRangedObjectIteratorAdapter(Components.begin(), Components.end());
    }

    virtual Ptr Append(const String& element) const
    {
      const StringArray& newOne = SplitPath(element);
      StringArray result(Components.size() + newOne.size());
      std::copy(newOne.begin(), newOne.end(),
        std::copy(Components.begin(), Components.end(), result.begin()));
      return boost::make_shared<ParsedPath>(result);
    }

    virtual Ptr Extract(const String& startPath) const
    {
      const StringArray& subSplitted = SplitPath(startPath);
      if (subSplitted.size() > Components.size())
      {
        return Ptr();
      }
      const std::pair<StringArray::const_iterator, StringArray::const_iterator> iters =
        std::mismatch(subSplitted.begin(), subSplitted.end(), Components.begin());
      if (iters.first != subSplitted.end())
      {
        return Ptr();
      }
      return boost::make_shared<ParsedPath>(iters.second, Components.end());
    }
  private:
    const StringArray Components;
  };
}

namespace Analysis
{
  Path::Ptr ParsePath(const String& str)
  {
    const StringArray& parts = SplitPath(str);
    return boost::make_shared<ParsedPath>(parts);
  }
}
