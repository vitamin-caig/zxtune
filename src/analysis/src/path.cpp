/**
* 
* @file
*
* @brief Analyzed data path implementation
*
* @author vitamin.caig@gmail.com
*
**/

//common includes
#include <contract.h>
#include <make_ptr.h>
//library includes
#include <analysis/path.h>
#include <strings/array.h>
//boost includes
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>
//std includes
#include <cassert>

namespace Analysis
{
  Strings::Array SplitPath(const String& str, Char separator)
  {
    const String delimiter(1, separator);
    Strings::Array parts;
    boost::algorithm::split(parts, str, boost::algorithm::is_any_of(delimiter), boost::algorithm::token_compress_on);
    const Strings::Array::iterator newEnd = std::remove_if(parts.begin(), parts.end(),
      std::mem_fun_ref(&String::empty));
    return Strings::Array(parts.begin(), newEnd);
  }

  String JoinPath(const Strings::Array& arr, Char separator)
  {
    const String delimiter(1, separator);
    return boost::algorithm::join(arr, delimiter);
  }

  template<class It>
  Path::Ptr CreatePath(Char separator, It from, It to);

  class ParsedPath : public Path
  {
  public:
    template<class Iter>
    ParsedPath(Char separator, Iter from, Iter to)
      : Components(from, to)
      , Separator(separator)
    {
      Require(from != to);
    }

    bool Empty() const override
    {
      return false;
    }

    String AsString() const override
    {
      return JoinPath(Components, Separator);
    }

    Iterator::Ptr GetIterator() const override
    {
      return CreateRangedObjectIteratorAdapter(Components.begin(), Components.end());
    }

    Ptr Append(const String& element) const override
    {
      const Strings::Array& newOne = SplitPath(element, Separator);
      Strings::Array result(Components.size() + newOne.size());
      std::copy(newOne.begin(), newOne.end(),
        std::copy(Components.begin(), Components.end(), result.begin()));
      return CreatePath(Separator, result.begin(), result.end());
    }

    Ptr Extract(const String& startPath) const override
    {
      const Strings::Array& subSplitted = SplitPath(startPath, Separator);
      if (subSplitted.size() > Components.size())
      {
        return Ptr();
      }
      const std::pair<Strings::Array::const_iterator, Strings::Array::const_iterator> iters =
        std::mismatch(subSplitted.begin(), subSplitted.end(), Components.begin());
      if (iters.first != subSplitted.end())
      {
        return Ptr();
      }
      return CreatePath(Separator, iters.second, Components.end());
    }
  private:
    const Strings::Array Components;
    const Char Separator;
  };

  class EmptyPath : public Path
  {
  public:
    explicit EmptyPath(Char separator)
      : Separator(separator)
    {
    }

    bool Empty() const override
    {
      return true;
    }

    String AsString() const override
    {
      return String();
    }

    Iterator::Ptr GetIterator() const override
    {
      return Iterator::CreateStub();
    }

    Ptr Append(const String& element) const override
    {
      return ParsePath(element, Separator);
    }

    Ptr Extract(const String& startPath) const override
    {
      return startPath.empty()
        ? MakePtr<EmptyPath>(Separator)
        : Ptr();
    }
  private:
    const Char Separator;
  };

  template<class It>
  Path::Ptr CreatePath(Char separator, It from, It to)
  {
    if (from != to)
    {
      return MakePtr<ParsedPath>(separator, from, to);
    }
    else
    {
      return MakePtr<EmptyPath>(separator);
    }
  }
}

namespace Analysis
{
  Path::Ptr ParsePath(const String& str, Char separator)
  {
    const Strings::Array parsed = SplitPath(str, separator);
    return CreatePath(separator, parsed.begin(), parsed.end());
  }
}
