/*
Abstract:
  Analyzed data path implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//common includes
#include <contract.h>
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
  StringArray SplitPath(const String& str, Char separator)
  {
    const String delimiter(1, separator);
    StringArray parts;
    boost::algorithm::split(parts, str, boost::algorithm::is_any_of(delimiter), boost::algorithm::token_compress_on);
    const StringArray::iterator newEnd = std::remove_if(parts.begin(), parts.end(),
      std::mem_fun_ref(&String::empty));
    return StringArray(parts.begin(), newEnd);
  }

  String JoinPath(const StringArray& arr, Char separator)
  {
    const String delimiter(1, separator);
    return boost::algorithm::join(arr, delimiter);
  }

  template<class It>
  Analysis::Path::Ptr CreatePath(Char separator, It from, It to);

  class ParsedPath : public Analysis::Path
  {
  public:
    template<class Iter>
    ParsedPath(Char separator, Iter from, Iter to)
      : Components(from, to)
      , Separator(separator)
    {
      Require(from != to);
    }

    virtual bool Empty() const
    {
      return false;
    }

    virtual String AsString() const
    {
      return JoinPath(Components, Separator);
    }

    virtual Iterator::Ptr GetIterator() const
    {
      return CreateRangedObjectIteratorAdapter(Components.begin(), Components.end());
    }

    virtual Ptr Append(const String& element) const
    {
      const StringArray& newOne = SplitPath(element, Separator);
      StringArray result(Components.size() + newOne.size());
      std::copy(newOne.begin(), newOne.end(),
        std::copy(Components.begin(), Components.end(), result.begin()));
      return CreatePath(Separator, result.begin(), result.end());
    }

    virtual Ptr Extract(const String& startPath) const
    {
      const StringArray& subSplitted = SplitPath(startPath, Separator);
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
      return CreatePath(Separator, iters.second, Components.end());
    }
  private:
    const StringArray Components;
    const Char Separator;
  };

  class EmptyPath : public Analysis::Path
  {
  public:
    explicit EmptyPath(Char separator)
      : Separator(separator)
    {
    }

    virtual bool Empty() const
    {
      return true;
    }

    virtual String AsString() const
    {
      return String();
    }

    virtual Iterator::Ptr GetIterator() const
    {
      return Iterator::CreateStub();
    }

    virtual Ptr Append(const String& element) const
    {
      return Analysis::ParsePath(element, Separator);
    }

    virtual Ptr Extract(const String& startPath) const
    {
      return startPath.empty()
        ? boost::make_shared<EmptyPath>(Separator)
        : Ptr();
    }
  private:
    const Char Separator;
  };

  template<class It>
  Analysis::Path::Ptr CreatePath(Char separator, It from, It to)
  {
    if (from != to)
    {
      return boost::make_shared<ParsedPath>(separator, from, to);
    }
    else
    {
      return boost::make_shared<EmptyPath>(separator);
    }
  }
}

namespace Analysis
{
  Path::Ptr ParsePath(const String& str, Char separator)
  {
    const StringArray parsed = SplitPath(str, separator);
    return CreatePath(separator, parsed.begin(), parsed.end());
  }
}
